package my12doom.x264client;

import android.app.Activity;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.Surface;
import android.view.Surface.OutOfResourcesException;
import android.view.SurfaceView;

public class X264clientActivity extends Activity {
	SurfaceView v;
	private Thread mWorker;
	private boolean mWorking;
	private core mCore = new core();

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
							mCore.startTest();
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
}