package my12doom.x264client;

import com.Vstar.Open3d;

import android.app.Activity;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.Surface.OutOfResourcesException;
import android.view.SurfaceView;

public class X264clientActivity extends Activity {
	SurfaceView v;
	private Thread mWorker;
	private boolean mWorking;
	private core mCore = new core();
	private Open3d mOpen3d;
	private boolean showing3d = false;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		v = (SurfaceView) findViewById(R.id.mainsurface);

		
	}
	
	public void onResume()
	{
		super.onRestart();
		
		mWorking = true;
		mWorker = new Thread() {
			public void run() {
				while (mWorking) {
					try {
						Thread.sleep(1);
						v.getHolder().setFormat(PixelFormat.RGB_565);
						Surface s = v.getHolder().getSurface();

						if (mCore.init(s) >=0)
						{
							mCore.startTest("192.168.1.199", 0xb03d);
							return;
						}

					} catch (Exception e) {
					}
				}
			}
		};
		mWorker.start();
	}
	
	public void onPause()
	{
		super.onPause();
		
		mWorking = false;
		try {
			mWorker.join();
		} catch (InterruptedException e) {}
		
		mCore.stopTest();
	}
	
	public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		if (keyCode == KeyEvent.KEYCODE_MENU)
		{
			if (mOpen3d == null)
				mOpen3d = new Open3d();
			showing3d = !showing3d;
			mOpen3d.Op3d(v, v.getHolder(), showing3d);
		}
		
		return super.onKeyDown(keyCode, event);
	}
}