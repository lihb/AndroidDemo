package cn.dennishucd;

/**
 * Created by Administrator on 2016/1/21.
 */
public class RtmpPush {

    static {
        System.loadLibrary("avutil-55");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("ffmpeg_codec");
    }


    public native int pushVideo(String inFilePath, String outFilePath);
}
