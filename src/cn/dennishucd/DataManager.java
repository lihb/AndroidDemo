package cn.dennishucd;

import android.graphics.Bitmap;

import java.util.ArrayList;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Created by lihb on 16/1/10.
 */
public class DataManager {

    public LinkedBlockingQueue<Bitmap> bitmapQueue = new LinkedBlockingQueue<Bitmap>(100);
    public ArrayList<Bitmap> arrayList = new ArrayList<Bitmap>();


    private static DataManager mInstance = null;

    private DataManager() {

    }

    public static synchronized DataManager getInstance(){
        if (mInstance == null) {
            mInstance = new DataManager();
        }
        return mInstance;
    }




}
