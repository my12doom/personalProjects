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
	
	
	static public DWindowNetworkConnection conn = new DWindowNetworkConnection();
	Button btn_go;
	Button btn_openfile;
	Button btn_openBD;
	Button btn_playpause;
	SeekBar sb_progress;
	EditText editHost;
	EditText editPassword;
	private Value<String> host = (Value<String>) Value.newValue("host", "192.168.1.199");
	private Value<String> password = (Value<String>) Value.newValue("password", "TestCode");
	private Bitmap bmp = null;
	private int total = 1;
	private int tell = 0;
	private boolean playing = false;
	
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
        btn_go = (Button)findViewById(R.id.btn_go);
        btn_openfile = (Button)findViewById(R.id.btn_open_file);
        btn_openBD = (Button)findViewById(R.id.btn_open_bd);
        btn_playpause = (Button)findViewById(R.id.btn_playpause);
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
				
				int result = connect();
				if (result == 1)
				{
					findViewById(R.id.login_layout).setVisibility(View.GONE);
					findViewById(R.id.main_console).setVisibility(View.VISIBLE);
					handler.sendEmptyMessageDelayed(0, 5);
				}
				else
				{
					Toast.makeText(SplashWindow3DActivity.this, result == 0 ? "Password Error" : "Connection Failed", Toast.LENGTH_SHORT).show();
				}
			}
        });
        
        btn_openfile.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectfileActivity.class);
				intent.putExtra("path", "\\");
				startActivityForResult(intent, request_code_openfile);
			}
        });
        
        btn_openBD.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(SplashWindow3DActivity.this, SelectfileActivity.class);
				intent.putExtra("path", "\\");
				intent.putExtra("BD", true);
				startActivityForResult(intent, request_code_openBD);
			}
        });
        
        btn_playpause.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				conn.execute_command("pause");
			}
        });
	}
    
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
    	if (requestCode == request_code_openfile || requestCode == request_code_openBD)
    	{
    		boolean selected = data.getBooleanExtra("selected", false);
    		String selected_file = data.getStringExtra("selected_file");
    		
    		if (selected)
    		{
    			tell = 0;
    			total = 1;
    			conn.execute_command("reset_and_loadfile|"+selected_file);
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
    			while(thisActivityVisible)
    			{
    				if (conn.getState() >= 0)
    				{
    		 			byte[] jpg = conn.shot();
    					if (jpg != null)
    						bmp = BitmapFactory.decodeByteArray(jpg, 0, jpg.length);
    				}
    				
    				try 
    				{
    					Thread.sleep(5);
	    				cmd_result result = conn.execute_command("tell");
	    				if (result.successed())
	    					tell = Integer.parseInt(result.result);
	    				result = conn.execute_command("total");
	    				if (result.successed())
	    					total = Integer.parseInt(result.result);
	    				result = conn.execute_command("is_playing");
	    				if (result.successed())
	    					playing = Boolean.parseBoolean(result.result);
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
    		btn_playpause.setText(playing?"Pause":"Play");
    		ImageView imageView = (ImageView)findViewById(R.id.iv_b512);
			if (conn.getState()<0)
				connect();
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
				if (total>0)sb_progress.setMax(total);
				if (tell>0)sb_progress.setProgress(tell);
			}
	   		handler.sendEmptyMessageDelayed(0, 20);
    	}
    };
    
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
    	if (keyCode == KeyEvent.KEYCODE_BACK)
    	{
    		if (conn.getState() >= 0)
    			conn.disconnect();
    		else
    			finish();
    	}
		return false;
    }
    
    private boolean isUserDragging = false;
    private int seek_target = -1;
    class SeekBarListener implements SeekBar.OnSeekBarChangeListener
    {
		public void onProgressChanged(SeekBar seekBar, int position, boolean fromUser) 
		{
			if (fromUser)
				seek_target = position;
		}
		public void onStartTrackingTouch(SeekBar seekBar)
		{
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
		}
	}
}