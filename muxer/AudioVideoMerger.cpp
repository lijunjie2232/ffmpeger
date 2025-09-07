#include "AudioVideoMerger.h"
#include <iostream>
#include <sstream>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

AudioVideoMerger::~AudioVideoMerger()
{
    if (videoFormatContext)
        avformat_close_input(&videoFormatContext);
    if (audioFormatContext)
        avformat_close_input(&audioFormatContext);
    if (outputFormatContext)
    {
        if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE))
        {
            avio_closep(&outputFormatContext->pb);
        }
        avformat_free_context(outputFormatContext);
    }
}

bool AudioVideoMerger::merge(const std::string &videoPath, const std::string &audioPath, const std::string &outputPath)
{
    lastError.clear();

    // 初始化FFmpeg库（在新版本中已弃用，但为了兼容性保留）
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    // 打开视频文件
    if (openInputFile(videoPath, &videoFormatContext) < 0)
    {
        setError("Failed to open video file: " + videoPath);
        return false;
    }

    // 打开音频文件
    if (openInputFile(audioPath, &audioFormatContext) < 0)
    {
        setError("Failed to open audio file: " + audioPath);
        return false;
    }

    // 创建输出文件
    if (createOutputFile(outputPath) < 0)
    {
        setError("Failed to create output file: " + outputPath);
        return false;
    }

    // format test
    // Print input files codec information
    printCodecInfo(videoFormatContext, "Video Input File (" + videoPath + ")");
    printCodecInfo(audioFormatContext, "Audio Input File (" + audioPath + ")");

    // 复制视频流
    if (copyOrCvtStreams(videoFormatContext, 0) < 0)
    {
        setError("Failed to copy video streams");
        return false;
    }

    // 复制音频流
    int audioStreamOffset = videoFormatContext->nb_streams;
    if (copyOrCvtStreams(audioFormatContext, audioStreamOffset) < 0)
    {
        setError("Failed to copy audio streams");
        return false;
    }

    // 写入输出文件头部
    if (avformat_write_header(outputFormatContext, nullptr) < 0)
    {
        setError("Failed to write file header");
        return false;
    }

    // 读取并写入数据包
    if (!processPackets(audioStreamOffset))
    {
        setError("Failed to process packets");
        return false;
    }

    // 写入文件尾部
    av_write_trailer(outputFormatContext);

    // Print output file codec information
    printOutputCodecInfo();
    return true;
}
void AudioVideoMerger::printCodecInfo(AVFormatContext *formatContext, const std::string &fileName)
{
    std::cout << "\n=== Codec Information for " << fileName << " ===" << std::endl;
    std::cout << "Format: " << formatContext->iformat->name << " (" << formatContext->iformat->long_name << ")" << std::endl;
    std::cout << "Number of streams: " << formatContext->nb_streams << std::endl;

    for (unsigned int i = 0; i < formatContext->nb_streams; i++)
    {
        AVStream *stream = formatContext->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        std::string codecName = codec ? codec->name : "Unknown";
        std::string codecType;

        switch (codecpar->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            codecType = "Video";
            std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
            std::cout << "  Resolution: " << codecpar->width << "x" << codecpar->height << std::endl;
            std::cout << "  Pixel format: " << av_get_pix_fmt_name((AVPixelFormat)codecpar->format) << std::endl;
            std::cout << "  Frame rate: " << av_q2d(stream->avg_frame_rate) << " fps" << std::endl;
            break;
        case AVMEDIA_TYPE_AUDIO:
            codecType = "Audio";
            std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
            std::cout << "  Sample rate: " << codecpar->sample_rate << " Hz" << std::endl;
            std::cout << "  Sample format: " << av_get_sample_fmt_name((AVSampleFormat)codecpar->format) << std::endl;
            std::cout << "  Bit rate: " << (codecpar->bit_rate > 0 ? std::to_string(codecpar->bit_rate) + " bps" : "Unknown") << std::endl;
            break;
        default:
            codecType = "Other";
            std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
            break;
        }

        std::cout << "  Codec ID: " << codecpar->codec_id << std::endl;
        std::cout << "  Time base: " << stream->time_base.num << "/" << stream->time_base.den << std::endl;
        if (codecpar->bit_rate > 0)
        {
            std::cout << "  Bit rate: " << codecpar->bit_rate << " bps" << std::endl;
        }
    }
}

void AudioVideoMerger::printOutputCodecInfo()
{
    std::cout << "\n=== Output File Codec Information ===" << std::endl;
    if (outputFormatContext->oformat)
    {
        std::cout << "Format: " << outputFormatContext->oformat->name << " (" << outputFormatContext->oformat->long_name << ")" << std::endl;
        std::cout << "Number of streams: " << outputFormatContext->nb_streams << std::endl;

        for (unsigned int i = 0; i < outputFormatContext->nb_streams; i++)
        {
            AVStream *stream = outputFormatContext->streams[i];
            AVCodecParameters *codecpar = stream->codecpar;

            const AVCodec *codec = avcodec_find_encoder(codecpar->codec_id);
            std::string codecName = codec ? codec->name : "Unknown";
            std::string codecType;

            switch (codecpar->codec_type)
            {
            case AVMEDIA_TYPE_VIDEO:
                codecType = "Video";
                std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
                std::cout << "  Resolution: " << codecpar->width << "x" << codecpar->height << std::endl;
                break;
            case AVMEDIA_TYPE_AUDIO:
                codecType = "Audio";
                std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
                std::cout << "  Sample rate: " << codecpar->sample_rate << " Hz" << std::endl;
                break;
            default:
                codecType = "Other";
                std::cout << "Stream #" << i << " - " << codecType << ": " << codecName << std::endl;
                break;
            }

            std::cout << "  Codec ID: " << codecpar->codec_id << std::endl;
            std::cout << "  Time base: " << stream->time_base.num << "/" << stream->time_base.den << std::endl;
        }
    }
}
int AudioVideoMerger::openInputFile(const std::string &filename, AVFormatContext **formatContext)
{
    *formatContext = nullptr;
    if (avformat_open_input(formatContext, filename.c_str(), nullptr, nullptr) < 0)
    {
        return -1;
    }

    if (avformat_find_stream_info(*formatContext, nullptr) < 0)
    {
        avformat_close_input(formatContext);
        return -1;
    }

    av_dump_format(*formatContext, 0, filename.c_str(), 0);
    return 0;
}

int AudioVideoMerger::createOutputFile(const std::string &filename)
{
    if (avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, filename.c_str()) < 0)
    {
        return -1;
    }

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&outputFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            return -1;
        }
    }

    return 0;
}

bool AudioVideoMerger::isStreamCompatible(AVStream* inStream, AVOutputFormat* outFormat) {
    // 1. 使用官方API检查
    int result = avformat_query_codec(outFormat, inStream->codecpar->codec_id, FF_COMPLIANCE_NORMAL);
    if (result == 1) {
        return true;
    }
    
    // 2. 检查编解码器标签支持
    if (outFormat->codec_tag) {
        unsigned int tag = av_codec_get_tag(outFormat->codec_tag, inStream->codecpar->codec_id);
        if (tag != 0) {
            return true;
        }
    }
    
    // 3. 特殊情况处理 - 某些格式可能需要转码
    // 例如用户可能想要强制转码以改变编码参数
    return false;
}

int AudioVideoMerger::copyOrCvtStreams(AVFormatContext *inputFormatCtx, int streamIndexOffset)
{
    for (unsigned int i = 0; i < inputFormatCtx->nb_streams; i++)
    {
        AVStream *inStream = inputFormatCtx->streams[i];
        AVStream *outStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!outStream)
        {
            return -1;
        }

        // 正确的转码判断：基于编解码器兼容性而不是容器格式
        if (isStreamCompatible(inStream, outputFormatContext->oformat))
        {
            // 编解码器兼容，直接复制参数
            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0)
            {
                return -1;
            }
            outStream->codecpar->codec_tag = 0;
            outStream->time_base = inStream->time_base;
        }
        else
        {
            // 需要转码 - 设置编解码器上下文
            // 这里需要实现实际的转码设置逻辑
            if (setupTranscoding(inStream, outStream) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

bool AudioVideoMerger::processPackets(int audioStreamOffset)
{
    AVPacket packet;

    // 处理视频数据包
    while (av_read_frame(videoFormatContext, &packet) >= 0)
    {
        if (packet.stream_index < (int)videoFormatContext->nb_streams)
        {
            AVStream *inStream = videoFormatContext->streams[packet.stream_index];
            AVStream *outStream = outputFormatContext->streams[packet.stream_index];

            // 转换时间戳
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.pos = -1;

            // 写入数据包
            if (av_interleaved_write_frame(outputFormatContext, &packet) < 0)
            {
                av_packet_unref(&packet);
                return false;
            }
        }
        av_packet_unref(&packet);
    }

    // 处理音频数据包
    while (av_read_frame(audioFormatContext, &packet) >= 0)
    {
        if (packet.stream_index < (int)audioFormatContext->nb_streams)
        {
            // 为音频流重新计算输出流索引
            int outStreamIndex = packet.stream_index + audioStreamOffset;
            AVStream *inStream = audioFormatContext->streams[packet.stream_index];
            AVStream *outStream = outputFormatContext->streams[outStreamIndex];

            packet.stream_index = outStreamIndex;

            // 转换时间戳
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.pos = -1;

            // 写入数据包
            if (av_interleaved_write_frame(outputFormatContext, &packet) < 0)
            {
                av_packet_unref(&packet);
                return false;
            }
        }
        av_packet_unref(&packet);
    }

    return true;
}