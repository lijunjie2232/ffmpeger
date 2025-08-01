#ifndef AUDIO_VIDEO_MERGER_H
#define AUDIO_VIDEO_MERGER_H

#include <string>
#include <vector>
extern "C" {
#include <libavformat/avformat.h>
}

class AudioVideoMerger {
private:
    AVFormatContext* videoFormatContext = nullptr;
    AVFormatContext* audioFormatContext = nullptr;
    AVFormatContext* outputFormatContext = nullptr;

public:
    ~AudioVideoMerger();

    /**
     * 合并音频和视频文件
     * @param videoPath 视频文件路径
     * @param audioPath 音频文件路径
     * @param outputPath 输出文件路径
     * @return 是否合并成功
     */
    bool merge(const std::string& videoPath, const std::string& audioPath, const std::string& outputPath);

    /**
     * 获取错误信息
     * @return 最后的错误信息
     */
    std::string getLastError() const { return lastError; }

private:
    std::string lastError;

    /**
     * 打开输入文件
     * @param filename 文件路径
     * @param formatContext 格式上下文指针
     * @return 成功返回0，失败返回负数
     */
    int openInputFile(const std::string& filename, AVFormatContext** formatContext);

    /**
     * 创建输出文件
     * @param filename 输出文件路径
     * @return 成功返回0，失败返回负数
     */
    int createOutputFile(const std::string& filename);

    /**
     * 复制流到输出文件
     * @param inputFormatCtx 输入格式上下文
     * @param streamIndex 流索引
     * @return 成功返回0，失败返回负数
     */
    int copyStreams(AVFormatContext* inputFormatCtx, int* streamIndex);

    /**
     * 处理并写入数据包
     * @return 成功返回true，失败返回false
     */
    bool processPackets();

    /**
     * 设置错误信息
     * @param error 错误信息
     */
    void setError(const std::string& error) { lastError = error; }
};

#endif // AUDIO_VIDEO_MERGER_H