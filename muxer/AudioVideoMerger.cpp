#include "AudioVideoMerger.h"
#include <iostream>
#include <sstream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

AudioVideoMerger::~AudioVideoMerger() {
    if (videoFormatContext) avformat_close_input(&videoFormatContext);
    if (audioFormatContext) avformat_close_input(&audioFormatContext);
    if (outputFormatContext) {
        if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outputFormatContext->pb);
        }
        avformat_free_context(outputFormatContext);
    }
}

bool AudioVideoMerger::merge(const std::string& videoPath, const std::string& audioPath, const std::string& outputPath) {
    lastError.clear();

    // 初始化FFmpeg库（在新版本中已弃用，但为了兼容性保留）
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif

    // 打开视频文件
    if (openInputFile(videoPath, &videoFormatContext) < 0) {
        setError("Failed to open video file: " + videoPath);
        return false;
    }

    // 打开音频文件
    if (openInputFile(audioPath, &audioFormatContext) < 0) {
        setError("Failed to open audio file: " + audioPath);
        return false;
    }

    // 创建输出文件
    if (createOutputFile(outputPath) < 0) {
        setError("Failed to create output file: " + outputPath);
        return false;
    }

    // 复制视频流
    int videoStreamIndex = -1;
    if (copyStreams(videoFormatContext, &videoStreamIndex) < 0) {
        setError("Failed to copy video streams");
        return false;
    }

    // 复制音频流
    int audioStreamIndex = -1;
    if (copyStreams(audioFormatContext, &audioStreamIndex) < 0) {
        setError("Failed to copy audio streams");
        return false;
    }

    // 写入输出文件头部
    if (avformat_write_header(outputFormatContext, nullptr) < 0) {
        setError("Failed to write file header");
        return false;
    }

    // 读取并写入数据包
    if (!processPackets()) {
        setError("Failed to process packets");
        return false;
    }

    // 写入文件尾部
    av_write_trailer(outputFormatContext);

    return true;
}

int AudioVideoMerger::openInputFile(const std::string& filename, AVFormatContext** formatContext) {
    *formatContext = nullptr;
    if (avformat_open_input(formatContext, filename.c_str(), nullptr, nullptr) < 0) {
        return -1;
    }

    if (avformat_find_stream_info(*formatContext, nullptr) < 0) {
        avformat_close_input(formatContext);
        return -1;
    }

    av_dump_format(*formatContext, 0, filename.c_str(), 0);
    return 0;
}

int AudioVideoMerger::createOutputFile(const std::string& filename) {
    if (avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, filename.c_str()) < 0) {
        return -1;
    }

    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputFormatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            return -1;
        }
    }

    return 0;
}

int AudioVideoMerger::copyStreams(AVFormatContext* inputFormatCtx, int* streamIndex) {
    for (unsigned int i = 0; i < inputFormatCtx->nb_streams; i++) {
        AVStream* inStream = inputFormatCtx->streams[i];
        AVStream* outStream = avformat_new_stream(outputFormatContext, nullptr);
        if (!outStream) {
            return -1;
        }

        *streamIndex = i;

        // 复制编解码器参数
        if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
            return -1;
        }
        outStream->codecpar->codec_tag = 0;
    }

    return 0;
}

bool AudioVideoMerger::processPackets() {
    AVPacket packet;

    // 处理视频数据包
    while (av_read_frame(videoFormatContext, &packet) >= 0) {
        if (packet.stream_index < (int)videoFormatContext->nb_streams) {
            AVStream* inStream = videoFormatContext->streams[packet.stream_index];
            AVStream* outStream = outputFormatContext->streams[packet.stream_index];

            // 转换时间戳
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.pos = -1;
            packet.stream_index = packet.stream_index; // 保持原索引

            // 写入数据包
            if (av_interleaved_write_frame(outputFormatContext, &packet) < 0) {
                av_packet_unref(&packet);
                return false;
            }
        }
        av_packet_unref(&packet);
    }

    // 处理音频数据包
    while (av_read_frame(audioFormatContext, &packet) >= 0) {
        if (packet.stream_index < (int)audioFormatContext->nb_streams) {
            // 为音频流重新计算输出流索引
            int outStreamIndex = packet.stream_index + videoFormatContext->nb_streams;
            AVStream* inStream = audioFormatContext->streams[packet.stream_index];
            AVStream* outStream = outputFormatContext->streams[outStreamIndex];

            // 保存原始stream_index用于时间戳转换
            int originalStreamIndex = packet.stream_index;
            packet.stream_index = outStreamIndex;

            // 转换时间戳
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.pos = -1;

            // 写入数据包
            if (av_interleaved_write_frame(outputFormatContext, &packet) < 0) {
                av_packet_unref(&packet);
                return false;
            }
        }
        av_packet_unref(&packet);
    }

    return true;
}