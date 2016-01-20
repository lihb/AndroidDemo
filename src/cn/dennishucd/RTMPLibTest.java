package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/20.
 */
public class RTMPLibTest {

    static {
        System.loadLibrary("rtmp");
        System.loadLibrary("ffmpeg_codec");
    }

    public native void NativeTest();
//    public native int InitSockets();
//    public native void CleanupSockets();


}
