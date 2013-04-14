package my12doom.x264client;

import android.view.Surface;

public class core {
	static{
		System.loadLibrary("core");
	}	
	
	public native int init(Surface surface);
	public native int uninit();
	public native int startTest(String host, int port);
	public native int stopTest();
	
	public core(){
		
	}	
}
