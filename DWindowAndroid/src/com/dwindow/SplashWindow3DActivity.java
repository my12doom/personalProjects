package com.dwindow;



import com.Vstar.DateBase;
import com.demo.splash.R;
import com.Vstar.Value;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Toast;

public class SplashWindow3DActivity extends Activity {
	
	static public DWindowNetworkConnection conn = new DWindowNetworkConnection();
	Button btn_GO;
	EditText editHost;
	EditText editPassword;
	private Value<String> host = (Value<String>) Value.newValue("host", "192.168.1.199");// 出入屏程度
	private Value<String> password = (Value<String>) Value.newValue("password", "TestCode");// 出入屏程度
	
	private boolean connect()
	{
		conn.connect(host.get());
		int login_result = conn.login(password.get());		
		return login_result == 1;
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
		super.onCreate(savedInstanceState);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.main);
		DateBase.Init(this);
		
        btn_GO = (Button)findViewById(R.id.btn_go);
        editHost = (EditText)findViewById(R.id.et_host);
        editPassword = (EditText)findViewById(R.id.et_password);
        editHost.setText(host.get());
        editPassword.setText(password.get());
        btn_GO.setOnClickListener(new OnClickListener()
        {
			public void onClick(View v) 
			{
				host.set(editHost.getText().toString());
				password.set(editPassword.getText().toString());
				
				if (connect())
				{
					findViewById(R.id.login_layout).setVisibility(View.GONE);
					handler.sendEmptyMessageDelayed(0, 5);
				}
				else
				{
					Toast.makeText(SplashWindow3DActivity.this, "Login Failed", Toast.LENGTH_SHORT).show();
				}
			}
        });
	}
    
    Handler handler = new Handler()
    {
    	public void handleMessage(Message msg)
    	{
    		ImageView imageView = (ImageView)findViewById(R.id.iv_b512);
			if (conn.getState()<0)
    		{
    	        findViewById(R.id.login_layout).setVisibility(View.VISIBLE);
				imageView.setImageResource(R.drawable.b512);
    			return;
    		}
   		
 			byte[] jpg = conn.shot();
			if (jpg != null)
			{
				Bitmap bmp = BitmapFactory.decodeByteArray(jpg, 0, jpg.length);
				if (bmp != null)
				{
					BitmapDrawable bd = new BitmapDrawable(bmp);
					
					//imageView.setBackgroundDrawable(bd);
					imageView.setImageBitmap(bmp);
				}
			}
	   		handler.sendEmptyMessageDelayed(0, 5);
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
}