package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/13.
 */
public class FFmpegAudioNative {

    static {
        System.loadLibrary("avutil-52");
        System.loadLibrary("avcodec-55");
        System.loadLibrary("swresample-0");
        System.loadLibrary("avformat-55");
        System.loadLibrary("swscale-2");
        System.loadLibrary("avfilter-4");
        System.loadLibrary("avdevice-55");
        System.loadLibrary("ffmpeg_codec");
    }

    public native int audioPlayer(String fileName);
    public native int audioPlayerPauseOrPlay();
    public native int audioPlayerStop();

}
