package cn.dennishucd;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.ImageView;

import java.util.concurrent.LinkedBlockingQueue;

/**
 * Created by Administrator on 2016/1/12.
 *
 * 该类仅将图片展示在一个imageview上,测试用
 */
public class Test extends Activity{

    private ImageView imageView = null;

    private LinkedBlockingQueue<Bitmap> queue =  null;
    private MyHandler myHandler = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.test);
        imageView = (ImageView) findViewById(R.id.img);

        queue = DataManager.getInstance().bitmapQueue;
        myHandler = new MyHandler();

        new Thread(new Runnable() {
            @Override
            public void run() {
                Bitmap bitmap = null;
                while (true) {
                    bitmap = queue.poll();
                    if (bitmap != null) {
                        Log.i("Test lihb test----", "poll()-->" + bitmap.toString());
                        Message message = Message.obtain();
                        message.obj = bitmap;
                        message.what = 1;
                        myHandler.removeCallbacksAndMessages(null);
                        myHandler.sendMessage(message);
                    }else {
                        Log.i("Test lihb test----", "poll()--> bitmap = null");
                    }

//                    try {
//                        Thread.sleep(10);
//                    } catch (InterruptedException e) {
//                        e.printStackTrace();
//                    }
                }

            }
        }).start();

    }

    private class MyHandler extends Handler{
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (msg.what == 1) {
                Bitmap bm = (Bitmap) msg.obj;
                imageView.setImageBitmap(bm);
            }

        }
    }
}
