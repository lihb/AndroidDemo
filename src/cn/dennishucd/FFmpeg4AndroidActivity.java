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
import android.widget.EditText;


public class FFmpeg4AndroidActivity extends Activity {

	private static final String TAG = "FFmpeg4AndroidActivity";
	private static final String PATH = "rtmp://183.61.143.98/flvplayback/mp4:panvideo1.mp4";
	private static final String PATH_OUT = Environment.getExternalStorageDirectory().getPath()
			+ "/1.pcm";
	private static final String PATH_OUT_FLV = Environment.getExternalStorageDirectory().getPath()
			+ "/ych.flv";

	private static final String PUSH_PATH = Environment.getExternalStorageDirectory().getPath() + "/1.mp4";
	private static final String PUSH_OUT = "rtmp://183.61.143.98:1935/flvplayback/testpush1";

	private Button btnStart = null;

	private EditText mRtmpEditTxt = null;

    private ThreadInJava threadInJava = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.main);

		btnStart = (Button) findViewById(R.id.button1);
		mRtmpEditTxt = (EditText) findViewById(R.id.rtmpEditTxt);


//		final RTMPLibTest rtmpLibTest = new RTMPLibTest();
		final FFmpegNative ffmpeg = new FFmpegNative(DataManager.getInstance().bitmapQueue);
		final FFmpegAudioNative fFmpegAudioNative = new FFmpegAudioNative();
//		final RtmpPush rtmpPush = new RtmpPush();

        threadInJava = new ThreadInJava(DataManager.getInstance().bitmapQueue);


		btnStart.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				String rtmpVideoPath = mRtmpEditTxt.getText().toString().trim();
				Log.i(TAG, "远程播放地址是:" + rtmpVideoPath);
				ffmpeg.naInit(rtmpVideoPath);
				ffmpeg.naSetup();
//				fFmpegAudioNative.startAudioPlayer(rtmpVideoPath, PATH_OUT);
				ffmpeg.naPlay();
//                threadInJava.startPlay(rtmpVideoPath);
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
