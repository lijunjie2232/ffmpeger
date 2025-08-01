#include "muxer.h"
#include "AudioVideoMerger.h"

int main(int argc, char* argv[]) {
    //if (argc != 4) {
    //    std::cerr << "用法: " << argv[0] << " <视频文件> <音频文件> <输出文件>" << std::endl;
    //    return -1;
    //}
    SetConsoleOutputCP(CP_UTF8);

    // 输入视频文件路径
    std::string videoFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_v.m4s";

    // 输入音频文件路径
    std::string audioFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_a.m4s";

    // 输出合并后的视频文件路径
    std::string outputFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test.mp4";

    AudioVideoMerger merger;
    if (merger.merge(videoFile, audioFile, outputFile)) {
        std::cout << "合并成功!" << std::endl;
        return 0;
    }
    else {
        std::cerr << "合并失败!" << std::endl;
        return -1;
    }
}