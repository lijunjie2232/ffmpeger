# test_avmerger.py
import avmerger

def main():
    merger = avmerger.AudioVideoMerger()
    
    # video_path = "input_video.mp4"
    # audio_path = "input_audio.mp3"
    # output_path = "output.mp4"
    """
    // 输入视频文件路径
    std::string videoFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_v.m4s";

    // 输入音频文件路径
    std::string audioFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_a.m4s";

    // 输出合并后的视频文件路径
    std::string outputFile = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test.mp4";
    """
    video_path = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_v.m4s"
    audio_path = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test_a.m4s"
    output_path = "C:\\Users\\25335\\AppData\\Roaming\\bilibili-core\\cache\\test.mp4"
    
    success = merger.merge(video_path, audio_path, output_path)
    
    if success:
        print("Merge successful!")
    else:
        error = merger.get_last_error()
        print(f"Merge failed: {error}")

if __name__ == "__main__":
    main()
