package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/20.
 */
public class RTMPLibTest {

    static {
        System.loadLibrary("rtmp-0");
        System.loadLibrary("ffmpeg_codec");
    }

    public native int naTest(String outFile);
//    public native int InitSockets();
//    public native void CleanupSockets();


}
