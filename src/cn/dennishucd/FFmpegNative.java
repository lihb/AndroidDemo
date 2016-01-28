package cn.dennishucd;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.*;
import java.util.concurrent.LinkedBlockingQueue;

public class FFmpegNative {

	private static LinkedBlockingQueue<Bitmap> bitmapQueue;

	public FFmpegNative(LinkedBlockingQueue<Bitmap> bitmapQueue) {
		this.bitmapQueue = bitmapQueue;
	}

	public native int naInit(String pFileName);
	public native int[] naGetVideoRes();
	public native int naSetup();

	public native void naPlay();

	public native int audioPlayer(String fileName);
	public native int audioPlayerPauseOrPlay();
	public native int audioPlayerStop();

	private void saveFrameToPath(Bitmap bitmap, String pPath) {
		Log.i("lihb test----- ", "saveFrameToPath() called");
		int BUFFER_SIZE = 1024 * 8;
		try {
			File file = new File(pPath);
			file.createNewFile();
			FileOutputStream fos = new FileOutputStream(file);
			final BufferedOutputStream bos = new BufferedOutputStream(fos, BUFFER_SIZE);
			bitmap.compress(Bitmap.CompressFormat.JPEG, 100, bos);
			bos.flush();
			bos.close();
			fos.close();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public static boolean offer(Bitmap bitmap) {
		Log.i("lihb test----- offfer()", bitmapQueue.toString());
		return bitmapQueue.offer(bitmap);
//		DataManager.getInstance().arrayList.add(bitmap);
//		Log.i("lihb test----- ", "offer()--called");
//		return true;
	}


	static {
		System.loadLibrary("avutil-55");
		System.loadLibrary("avcodec-57");
		System.loadLibrary("swresample-2");
		System.loadLibrary("avformat-57");
		System.loadLibrary("swscale-4");
		System.loadLibrary("avfilter-6");
		System.loadLibrary("ffmpeg_codec");
	}






}
