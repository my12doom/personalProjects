/*
 * 已知问题：
 * 
 * 1、Sharp手机在开/关3D后会重新创建Surface并产生surfaceCreated事件，所以如果在surfaceCreated中调用的话会造成死循环
 *    其他手机必须要在surfaceCreated事件之后再开/关3D才会起效
 * 解决：
 *    不要再surfaceCreated/surfaceDestroyed事件中开/关3D
 *    开/关3D用一个线程等待surfaceCreated事件，并用handler传回主线程调用 
 *    示例代码：
 *    private boolean mSurfaceCreated = false;
 *    public void surfaceCreated(SurfaceHolder holder) {mSurfaceCreated = true;}
 *    public void surfaceDestroyed(SurfaceHolder holder) {mSurfaceCreated = false;}
 *    private Handler open3dHandler = new Handler(){public void handleMessage(Message msg) {open3d.Op3d(surfaceView, surfaceHolder, msg.what==1);}}
 *    private void open3d(final boolean open){
 *    	new Thread() {
			public void run() {
				while(!mSurfaceCreated) Thread.sleep(1);
				open3dHandler.sendEmptyMessage(open?1:0);
			}
		}.start();
 *    }
 *    
	};
 *   
 * 2、EVO3D某版4.03的固件在开/关3D后不会立即生效
 * 解决：
 *    开/关3D后弹出任意一个窗口/toast才会生效，可以弹出个toast来生效
 */

package com.Vstar;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class Open3d {
	static{
		System.loadLibrary("3dvOpen3d");
	}
	
	private int Brand3DNo=0;
	private int Brand3DHTC=1;
	private int Brand3DLG=2;
	private int Brand3DSHAP=3;
	private int Brand3d=0;
	
	private native int Get3DBrand(ClassLoader pClassLoader);	
	private native int Open3D(boolean show,SurfaceView mSurfaceView,SurfaceHolder holder,Surface mSurface,boolean issysplay,boolean is3dv, ClassLoader loader);
	public native void delobj();
	private native boolean IsPLR3dv(String path);
	private native Object GetShartObject(Object sv);
	
	public Open3d(){
		try {
			Brand3d = Get3DBrand(getClass().getClassLoader());
		} catch (Exception e) {
		}
	}
	
	public void Op3d(SurfaceView faceview,SurfaceHolder holder,Boolean show3d) {		
		try{
			Open3D(show3d,faceview,holder,holder.getSurface(),false,false, getClass().getClassLoader());
		}catch (Exception e2) {			
		}catch (Error e) {			
		}
}
	
	/**
	 * 判断是否是裸眼3D手机
	 * @return
	 */
	public Boolean has3D() {
		if(Brand3d==0)
			return false;
		return true;
	}
}
