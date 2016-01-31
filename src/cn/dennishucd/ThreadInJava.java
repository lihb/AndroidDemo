package cn.dennishucd;

import android.graphics.Bitmap;
import android.util.Log;

import java.util.concurrent.LinkedBlockingQueue;

/**
 * Created by Administrator on 2016/1/29.
 */
public class ThreadInJava {

    private static LinkedBlockingQueue<Bitmap> bitmapQueue;

    public ThreadInJava(LinkedBlockingQueue<Bitmap> bitmapQueue) {
        this.bitmapQueue = bitmapQueue;
    }


    public void startPlay(final String fileName) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                nativePlay(fileName);

            }
        }).start();
    }

    public native int nativePlay(String fileName);


    public static boolean offerInJava(Bitmap bitmap) {
        Log.i("lihb--offerInJava()", bitmapQueue.toString());
        return bitmapQueue.offer(bitmap);
    }

    static {
        System.loadLibrary("avutil-52");
        System.loadLibrary("avcodec-55");
        System.loadLibrary("avformat-55");
        System.loadLibrary("swscale-2");
        System.loadLibrary("swresample-0");
        System.loadLibrary("ffmpeg_codec");
    }

}
