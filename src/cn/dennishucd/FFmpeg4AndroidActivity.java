package cn.dennishucd;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Point;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.Button;

public class FFmpeg4AndroidActivity extends Activity {

	private static final String TAG = "FFmpeg4AndroidActivity";
	private static final String PATH = Environment.getExternalStorageDirectory().getPath()
			+ "/1.mp4";
	private static final String PATH_OUT = Environment.getExternalStorageDirectory().getPath()
			+ "/1.pcm";

	private Button btnStart = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.main);

		btnStart = (Button) findViewById(R.id.button1);

		final FFmpegNative ffmpeg = new FFmpegNative(DataManager.getInstance().bitmapQueue);
		final FFmpegAudioNative fFmpegAudioNative = new FFmpegAudioNative();

		ffmpeg.naInit(PATH);
		int[] resArr = ffmpeg.naGetVideoRes();
		Log.d(TAG, "res width " + resArr[0] + ": height " + resArr[1]);
		int[] screenRes = getScreenRes();
		int width, height;
		float widthScaledRatio = screenRes[0]*1.0f / resArr[0];
		float heightScaledRatio = screenRes[1]*1.0f / resArr[1];
		if (widthScaledRatio > heightScaledRatio) {
			//use heightScaledRatio
			width = (int) (resArr[0]*heightScaledRatio);
			height = screenRes[1];
		} else {
			//use widthScaledRatio
			width = screenRes[0];
			height = (int) (resArr[1]*widthScaledRatio);
		}
		Log.d(TAG, "width " + width + ",height:" + height);
		ffmpeg.naSetup(1920, 960);


		btnStart.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				fFmpegAudioNative.startAudioPlayer(PATH, PATH_OUT);
				ffmpeg.naPlay();
				Intent intent = new Intent(FFmpeg4AndroidActivity.this, PanoramicView.class);
				startActivity(intent);
			}
		});

	}

	@SuppressLint("NewApi")
	private int[] getScreenRes() {
		int[] res = new int[2];
		Display display = getWindowManager().getDefaultDisplay();
		if (Build.VERSION.SDK_INT >= 13) {
			Point size = new Point();
			display.getSize(size);
			res[0] = size.x;
			res[1] = size.y;
		} else {
			res[0] = display.getWidth();  // deprecated
			res[1] = display.getHeight();  // deprecated
		}
		return res;
	}
}
