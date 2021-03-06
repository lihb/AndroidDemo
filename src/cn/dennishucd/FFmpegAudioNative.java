package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/13.
 */
public class FFmpegAudioNative {

    static {
        System.loadLibrary("avutil-52");
        System.loadLibrary("avcodec-55");
        System.loadLibrary("avformat-55");
        System.loadLibrary("swscale-2");
        System.loadLibrary("swresample-0");
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
