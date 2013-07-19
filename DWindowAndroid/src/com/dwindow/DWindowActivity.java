package com.dwindow;



import java.util.Vector;

import com.demo.splash.R;
import com.dwindow.DWindowNetworkConnection.Track;
import com.dwindow.DWindowNetworkConnection.cmd_result;
import com.dwindow.util.DateBase;
import com.dwindow.util.Value;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;

public class DWindowActivity extends Activity {
	
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
	Button btn_shutdown;
	Button btn_swapeyes;
	SeekBar sb_progress;
	SeekBar sb_volume;
	AutoCompleteTextView editHost;
	EditText editPassword;
	private Value<String> host;
	private Value<String> password;
	public static Value<String> path;
	private Bitmap bmp = null;
	private int total = 1;
	private int tell = 0;
	private int volume = 100;
	private boolean playing = false;
	private boolean disconnectIsFromUser = false;
	private boolean is2D = false;
	private boolean thisActivityVisible = false; 
	private Thread shotThread;
	private Vector<String>availableIPs;
	
	private int connect()
	{
		conn.connect(host.get());
		return conn.login(password.get());
	}
	
	class ScannerThread implements Runnable 
	{
		private String host;
		public ScannerThread(String host)
		{
			this.host = host;
		}
		public void run() 
		{
			DWindowNetworkConnection connection = new DWindowNetworkConnection();
			if (connection.connect(host))
			{
				availableIPs.add(host);
				autoCompleteUpdateHandler.sendEmptyMessage(0);
			}
			connection.disconnect();
		}
	}
	
	private void login()
	{
		InputMethodManager inputMethodManager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		inputMethodManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
		
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
			String hint = getResources().getString(R.string.ConnectionFailed);
			if (result == 0 )
				hint = getResources().getString(R.string.PasswordError);
			if (state == conn.ERROR_NEW_PROTOCOL)
				hint = getResources().getString(R.string.NeedNewVersion);
			
			Toast.makeText(DWindowActivity.this, hint, Toast.LENGTH_SHORT).show();
		}
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
		super.onCreate(savedInstanceState);
		//getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		//requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.main);
		DateBase.Init(this);
		host = (Value<String>) Value.newValue("host", "");
		password = (Value<String>) Value.newValue("password", "");
		path = (Value<String>) Value.newValue("path", "\\");
		
		
		
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
        btn_shutdown = (Button)findViewById(R.id.btn_shutdown);
        btn_swapeyes = (Button)findViewById(R.id.btn_swapeyes);
        editHost = (AutoCompleteTextView)findViewById(R.id.et_host);
        editPassword = (EditText)findViewById(R.id.et_password);
        editPassword.setImeOptions(EditorInfo.IME_ACTION_DONE);
        editPassword.setOnEditorActionListener(new OnEditorActionListener()
        {
			public boolean onEditorAction(TextView v, int actionId,KeyEvent event)
			{
				login();
				return false;
			}
		});
        editHost.setText(host.get());
        editPassword.setText(password.get());
        btn_go.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				login();
			}
        });
        
        btn_openfile.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(DWindowActivity.this, SelectfileActivity.class);
				intent.putExtra("path", path.get());
				startActivityForResult(intent, request_code_openfile);
			}
        });
        
        btn_openBD.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(DWindowActivity.this, SelectfileActivity.class);
				intent.putExtra("path", path.get());
				intent.putExtra("BD", true);
				startActivityForResult(intent, request_code_openBD);
			}
        });
        
        btn_selectaudio.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(DWindowActivity.this, SelectTrackActivity.class);
				intent.putExtra("track", "audio");
				startActivityForResult(intent, request_code_selectAudio);
			}
        });

        btn_selectsubtitle.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				Intent intent = new Intent(DWindowActivity.this, SelectTrackActivity.class);
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
        
        btn_swapeyes.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				cmd_result result = conn.execute_command("get_swapeyes");
				if (result.successed())
				{
					boolean b = false;
					try
					{
						b = Boolean.parseBoolean(result.result);
					}catch(Exception e){}
					
					conn.execute_command("set_swapeyes|"+!b);
				}
			}
        });
        
        btn_shutdown.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				disconnectIsFromUser = true;
				conn.execute_command("shutdown");
			}
        });
        
        
        Button btn_toggle2d = (Button)findViewById(R.id.btn_toggle2d);
        btn_toggle2d.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				is2D = !is2D;
				conn.execute_command("set_output_mode|" + (is2D?"3":"11"));
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
    	}
    }
    
    public void onResume()
    {
    	try
		{
    		availableIPs = new Vector<String>();
			int ip = ((WifiManager)getSystemService(Context.WIFI_SERVICE)).getConnectionInfo().getIpAddress();
			String ipstring = ( ip & 0xFF)+ "." + ((ip >> 8 ) & 0xFF) + "." + ((ip >> 16 ) & 0xFF) + ".";
			for(int i=1; i<255; i++)
				new Thread(new ScannerThread(ipstring+i)).start();
		}catch(Exception e){}
		
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
    Handler autoCompleteUpdateHandler = new Handler()
    {
    	public void handleMessage(Message msg)
    	{
    		ArrayAdapter<String> adapter=new ArrayAdapter<String>(DWindowActivity.this,
            		android.R.layout.simple_dropdown_item_1line, (String[])availableIPs.toArray(new String[availableIPs.size()]));
    		editHost.setAdapter(adapter);
    		editHost.setThreshold(0);
    	}
    };
    
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
				imageView.setImageResource(R.drawable.logo);
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