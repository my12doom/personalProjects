package com.dwindow;



import com.Vstar.DateBase;
import com.demo.splash.R;
import com.dwindow.DWindowNetworkConnection.cmd_result;
import com.Vstar.Value;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.Toast;

public class SplashWindow3DActivity extends Activity {
	
	private final int request_code_openfile = 10000;
	private final int request_code_openBD = 10001;
	private final int request_code_selectAudio = 10002;
	private final int request_code_selectSubtitle = 10003;
	
	
	static public DWindowNetworkConnection conn = new DWindowNetworkConnection();
	Button btn_go;
	Button btn_openfile;
	Button btn_openBD;
	Button btn_playpause;
	Button btn_fullscreen;
	Button btn_selectaudio;
	Button btn_selectsubtitle;
	SeekBar sb_progress;
	SeekBar sb_volume;
	EditText editHost;
	EditText editPassword;
	private Value<String> host = (Value<String>) Value.newValue("host", "192.168.1.199");
	private Value<String> password = (Value<String>) Value.newValue("password", "TestCode");
	private Value<String> path = (Value<String>) Value.newValue("path", "\\");
	private Bitmap bmp = null;
	private int total = 1;
	private int tell = 0;
	private int volume = 100;
	private boolean playing = false;
	private boolean disconnectIsFromUser = false;
	
	private boolean thisActivityVisible = false; 
	private Thread shotThread;
	
	private int connect()
	{
		conn.connect(host.get());
		return conn.login(password.get());
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
		super.onCreate(savedInstanceState);
		//getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		//requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.main);
		DateBase.Init(this);
		
        sb_progress = (SeekBar)findViewById(R.id.sb_progress);
        sb_progress.setOnSeekBarChangeListener(new SeekBarListener());
        sb_volume = (SeekBar)findViewById(R.id.sb_volume);
        sb_volume.setOnSeekBarChangeListener(new SeekBarListener());
        btn_go = (Button)findViewById(R.id.btn_go);
        btn_openfile = (Button)findViewById(R.id.btn_open_file);
        btn_openBD = (Button)findViewById(R.id.btn_open_bd);
        btn_playpause = (Button)findViewById(R.id.btn_playpause);
        btn_fullscreen = (Button)findViewById(R.id.btn_fullscreen);
        btn_selectaudio = (Button)findViewById(R.id.btn_audiotrack);
        btn_selectsubtitle = (Button)findViewById(R.id.btn_subtitletrack);
        editHost = (EditText)findViewById(R.id.et_host);
        editPassword = (EditText)findViewById(R.id.et_password);
        editHost.setText(host.get());
        editPassword.setText(password.get());
        btn_go.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				host.set(editHost.getText().toString());
				password.set(editPassword.getText().toString());
				disconnectIsFromUser = false;
				
				int result = connect();
				if (result == 1)
				{
					findViewById(R.id.login_layout).setVisibility(View.GONE);
					findViewById(R.id.main_console).setVisibility(View.VISIBLE);
					handler.sendEmptyMessageDelayed(0, 5);
				}
				else
				{
					int state = conn.getState();
					String hint = "Connection Failed";
					if (result == 0 )
						hint = "Password Error";
					if (state == conn.ERROR_NEW_PROTOCOL)
						hint = "Need New Version";
					
					Toast.makeText(SplashWindow3DActivity.this, hint, Toast.LENGTH_SHORT).show();
				}
			}
        });
        
        btn_openfile.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectfileActivity.class);
				intent.putExtra("path", path.get());
				startActivityForResult(intent, request_code_openfile);
			}
        });
        
        btn_openBD.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectfileActivity.class);
				intent.putExtra("path", path.get());
				intent.putExtra("BD", true);
				startActivityForResult(intent, request_code_openBD);
			}
        });
        
        btn_selectaudio.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectTrackActivity.class);
				intent.putExtra("track", "audio");
				startActivityForResult(intent, request_code_selectAudio);
			}
        });

        btn_selectsubtitle.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectTrackActivity.class);
				intent.putExtra("track", "subtitle");
				startActivityForResult(intent, request_code_selectSubtitle);
			}
        });
        btn_playpause.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				conn.execute_command("pause");
			}
        });
        
        btn_fullscreen.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				conn.execute_command("toggle_fullscreen");
			}
        });
}
    
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
    	if (requestCode == request_code_openfile || requestCode == request_code_openBD)
    	{
    		boolean selected = data.getBooleanExtra("selected", false);
    		String selected_file = data.getStringExtra("selected_file");
    		path.set(data.getStringExtra("path"));
    		
    		if (selected)
    		{
    			tell = 0;
    			total = 1;
    			conn.execute_command("reset_and_loadfile|"+selected_file);
    		}
    	}
    	else if (requestCode == request_code_selectAudio || requestCode == request_code_selectSubtitle)
    	{
    		boolean selected = data.getBooleanExtra("selected", false);
    		int selected_track = data.getIntExtra("selected_track", -999);
    		
    		if (selected)
    		{
    			conn.execute_command("enable_" + (requestCode == request_code_selectAudio ? "audio" : "subtitle") +"_track|"+selected_track);
    		}   		
    	}
    }
    
    public void onResume()
    {
    	thisActivityVisible = true;
    	shotThread = new Thread()
    	{
    		public void run()
    		{
    			long last_UI_fetch = System.currentTimeMillis();
    			while(thisActivityVisible)
    			{
    				if (conn.getState() >= 0)
    				{
    					System.out.println("shot() start");
    		 			byte[] jpg = null;
    		 			jpg = conn.shot();
    					System.out.println("shot() network end");
    					if (jpg != null)
    						bmp = BitmapFactory.decodeByteArray(jpg, 0, jpg.length);
    					System.out.println("shot() end");
    				}
    				
    				try 
    				{
    					Thread.sleep(5);
    					
    					if (System.currentTimeMillis() - last_UI_fetch > 3000)
    					{
		    				cmd_result result = conn.execute_command("tell");
		    				if (result.successed() && total > 1)
		    					tell = Integer.parseInt(result.result);
		    				result = conn.execute_command("total");
		    				if (result.successed())
		    					total = Integer.parseInt(result.result);
		    				result = conn.execute_command("get_volume");
		    				if (result.successed())
		    					volume = (int) (Float.parseFloat(result.result) * 100);
		    				result = conn.execute_command("is_playing");
		    				if (result.successed())
		    					playing = Boolean.parseBoolean(result.result);
 	    					last_UI_fetch = System.currentTimeMillis();
    					}
    				}
    				catch (Exception e) 
    				{
    				}
    			}
    		}
    	};
    	shotThread.start();
    	super.onResume();
    }
    
    public void onPause()
    {
    	thisActivityVisible = false;
    	try 
    	{
			shotThread.join();
		} catch (InterruptedException e) 
		{
		}
    	super.onPause();
    }
    
    Handler handler = new Handler()
    {
    	public void handleMessage(Message msg)
    	{
			if (conn.getState()<0 && !disconnectIsFromUser)
				connect();
			
    		ImageView imageView = (ImageView)findViewById(R.id.iv_b512);
    		btn_playpause.setText(playing?"Pause":"Play");
			if (conn.getState()<0)
    		{
				findViewById(R.id.login_layout).setVisibility(View.VISIBLE);
				findViewById(R.id.main_console).setVisibility(View.GONE);
				imageView.setImageResource(R.drawable.b512);
    			return;
    		}
   		
			if (bmp != null)
			{
				imageView.setImageBitmap(bmp);
				bmp = null;
			}
			if (!isUserDragging)
			{
				if (total>=0)sb_progress.setMax(total);
				if (tell>=0)sb_progress.setProgress(tell);
				if (volume>=0)sb_volume.setProgress(volume);
				sb_volume.setMax(100);
				
				System.out.println(String.format("progress: %d / %d", tell, total));
			}
	   		handler.sendEmptyMessageDelayed(0, 5);
    	}
    };
    
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
    	if (keyCode == KeyEvent.KEYCODE_BACK)
    	{
    		disconnectIsFromUser = true;
    		if (conn.getState() >= 0)
    			conn.disconnect();
    		else
    			finish();
    	}
		return false;
    }
    
    private boolean isUserDragging = false;
    private int seek_target = -1;
    private int volume_target = -1;
    class SeekBarListener implements SeekBar.OnSeekBarChangeListener
    {
		public void onProgressChanged(SeekBar seekBar, int position, boolean fromUser) 
		{
			if (fromUser)
			{
				if (seekBar == sb_progress)
					seek_target = position;
				else
					volume_target = position;
			}
		}
		public void onStartTrackingTouch(SeekBar seekBar)
		{
			volume_target = -1;
			seek_target = -1;
			isUserDragging = true;
		}
		public void onStopTrackingTouch(SeekBar seekBar)
		{
			isUserDragging = false;
			if (seek_target>=0)
			{
				total = tell = -1;
				conn.execute_command("seek|"+seek_target);
			}
			if (volume_target >= 0)
			{
				volume = -1;
				conn.execute_command("set_volume|"+(float)volume_target/100f);
			}
		}
	}
}