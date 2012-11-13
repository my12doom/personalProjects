package com.Vstar;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.net.Uri;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public class Splash implements SurfaceHolder.Callback{
	
	public interface OnSplashCompletionListener{
		void onCompletion();
	}
	
	private SurfaceView mSurfaceView;
	private MediaPlayer mMediaPlayer;
	private String mClip2d;
	private String mClip3d;
	private OnSplashCompletionListener mListener;
	private boolean mSurfaceNotCreated = true;
	private Open3d mOpen3d;
	private Context mContext;
	
	public Splash(Context context, SurfaceView surfaceView, String clip3d, String clip2d) {
		mContext = context;
		mOpen3d = new Open3d();
		mClip2d = clip2d;
		mClip3d = clip3d;		
		mSurfaceView=surfaceView;	
		mMediaPlayer = new MediaPlayer();
		mMediaPlayer.reset();
		mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
		SurfaceHolder holder = mSurfaceView.getHolder();
		holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		holder.addCallback(this);
	}
	
	public void setOnCompletionListener(OnSplashCompletionListener listener){
		mListener=listener;
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (mOpen3d.has3D() && mSurfaceNotCreated) {
			mOpen3d.Op3d(mSurfaceView, holder, true);
			mSurfaceNotCreated = false;
		}
	}

	public void surfaceCreated(SurfaceHolder holder) {
		mMediaPlayer.setOnCompletionListener(new OnCompletionListener() {
			public void onCompletion(MediaPlayer mp) {
				mp.reset();
				mp.release();
				if(mOpen3d!=null)
					mOpen3d.Op3d(mSurfaceView, mSurfaceView.getHolder(), false);
				mMediaPlayer.release();
				mSurfaceView.setVisibility(View.GONE);
				
				if(mListener!=null){
					mListener.onCompletion();
				}
			}
		});

		try {
			mOpen3d.Op3d(mSurfaceView, holder, true);
			mSurfaceNotCreated = false;
			mMediaPlayer.setDisplay(holder);
			mMediaPlayer.setDataSource(mContext, Uri.parse(mOpen3d.has3D() ? mClip3d : mClip2d));
			mMediaPlayer.prepare();
			mMediaPlayer.start();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	public void surfaceDestroyed(SurfaceHolder holder) {
		if(mOpen3d!=null)
			mOpen3d.Op3d(mSurfaceView, holder, false);
		mMediaPlayer.release();
		mSurfaceView.setVisibility(View.GONE);
	}
}
