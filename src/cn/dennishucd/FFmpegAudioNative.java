package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/13.
 */
public class FFmpegAudioNative {

    static {
        System.loadLibrary("avutil-55");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("ffmpeg_codec");
    }
    public native int audioPlayerPauseOrPlay();
    public native int audioPlayerStop();
    public  native void setPlayingAudioPlayer(boolean isPlaying);

    public  native void setVolumeAudioPlayer(int millibel);

    public  native void setMutAudioPlayer(boolean mute);
    public  native void startAudioPlayer(String fileName, String outFileName);


    public  native void shutdown();

}
