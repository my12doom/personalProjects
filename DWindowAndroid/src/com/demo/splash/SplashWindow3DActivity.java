package com.demo.splash;


import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;

import com.Vstar.Splash;
import com.Vstar.Splash.OnSplashCompletionListener;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import java.util.Comparator;

public class SplashWindow3DActivity extends Activity {
	
	String[] file_list = new String[]{"connection failed"};
	ListViewAdapter adapter;
	ListView listView;
	
	class HRESULT{
		public long m_code;
		public HRESULT(String str){
			m_code = Long.valueOf(str,16);
		}
		
		public boolean failed(){
			return m_code>=0x8000000;
		}
		
		public String toString(){
			return String.format("%08x", m_code);
		}
		
		public boolean successed(){
			return !failed();
		}
	}
	
	class cmd_result{
		cmd_result(){
			hr = new HRESULT("80004005");	// E_FAIL
		}
		HRESULT hr;
		String result;
		public boolean failed(){
			return hr.failed();
		}
		
		public String toString(){
			return hr.toString();
		}
		
		public boolean successed(){
			return hr.successed();
		}
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main);
        
        SurfaceView surface_wel=(SurfaceView)findViewById(R.id.surfaceview_splash);
        listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener(){
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
        		String file = file_list[position];
        		if (!file.endsWith("\\"))
        			send_cmd("reset_and_loadfile|"+path.substring(1,path.length())+file);
        		else
        		{
        			path += file;
        			refresh();
        			listView.setSelection(0);
        		}
			}
        });
        
        listView.setDividerHeight(0);
        surface_wel.setVisibility(View.VISIBLE);
        
		String clip2d = "android.resource://" + getPackageName() + "/" + R.raw.splash2d;
		String clip3d = "android.resource://" + getPackageName() + "/" + R.raw.splash3d;

		Splash splash = new Splash(this, surface_wel, clip3d, clip2d);
		splash.setOnCompletionListener(new OnSplashCompletionListener() {
			public void onCompletion() {
				setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
				cmd_result isfull = send_cmd2("is_fullscreen");
				if (isfull.successed() && !isfull.result.equalsIgnoreCase("true"))
					send_cmd("toggle_fullscreen");
				
				refresh();
				listView.setSelection(0);
			}
		});
		
		System.out.println(send_cmd2("shit").hr.successed());
	}
    
    private boolean isMediaFile(String filename){
    	String[] exts = {".mp4", ".mkv", ".avi", ".rmvb", ".wmv", ".avs", ".ts", ".m2ts", ".ssif", ".mpls", ".3dv", ".e3d"};
    	
    	for(int i=0; i<exts.length; i++)
    		if (filename.endsWith(exts[i]))
    			return true;
    	return false;
    }
    
    private String path = "\\";
    private void refresh(){
		cmd_result list_result = send_cmd2("list_file|"+path);
		if (list_result.successed())
			file_list = list_result.result.split("\\|");				
		
		Arrays.sort(file_list, new Comparator<String>(){
			public int compare(String lhs, String rhs) {
				if (lhs.endsWith("\\") != rhs.endsWith("\\"))
					return lhs.endsWith("\\") ? -1 : 1;
				
				if (isMediaFile(lhs) && isMediaFile(rhs))
					;
				else{
					if (isMediaFile(lhs))
						return -1;
					if (isMediaFile(rhs))
						return 1;
				}				
				
				return lhs.compareTo(rhs);
			}
		});
		
		adapter.notifyDataSetChanged();   	
    }
    
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (path.length() > 1){
				path = path.substring(0,path.lastIndexOf("\\"));
				path = path.substring(0,path.lastIndexOf("\\")+1);
				refresh();
				
				return true;
			}
			else{
				finish();
			}
		} else if (keyCode == KeyEvent.KEYCODE_MENU){
			refresh();
		}
		return false;
    }
    
    public void onStop(){
    	send_cmd("reset");
    	super.onStop();
    }
    
    private cmd_result send_cmd2(String cmd){
    	cmd_result out = new cmd_result();
    	try{
    	String file_list_str = send_cmd(cmd);
		String[] split = file_list_str.split(",", 2);
		
		out.result = split.length > 1 ? split[1] : "";
		out.hr = new HRESULT(split[0]);
    	}catch (Exception e){    		
    	}
		
    	return out;
    }
    
    private String send_cmd(String cmd){
    	Log.i("send_cmd()", "sending "+cmd);
    	String out = null;
        Socket socket = null;
        try {
            String msg = cmd+"\r\n";
            socket = new Socket();
            InetAddress addr = InetAddress.getByName( "192.168.1.199");
            socket.connect(new InetSocketAddress(addr, 8080), 3000);
            InputStream inputStream = socket.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream, "UTF-8"));
            String welcomeString = reader.readLine();
            
            OutputStream outputStream = socket.getOutputStream();
            outputStream.write(msg.getBytes("UTF-8"));
            out = reader.readLine();
        } catch (UnknownHostException e) {
        } catch (IOException e) {
        } catch (Exception e){
        } finally {
            try {socket.close();} catch (Exception e){}
        }
        
    	Log.i("send_cmd()", "got "+out);
        return out;
    }
    
    private class ListViewAdapter extends BaseAdapter {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) {
			inflater = LayoutInflater.from(context);
		}

		@Override
		public int getCount() {
			return file_list.length;
		}

		@Override
		public Object getItem(int position) {
			return null;
		}

		@Override
		public long getItemId(int position) {
			return 0;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			if (convertView == null)
				convertView = inflater.inflate(R.layout.lv_open_item, null);
			TextView tv_filename = (TextView) convertView.findViewById(R.id.tv_open_filename);
			String file = file_list[position];
			if (file.endsWith("\\"))
				file = "("+file.substring(0, file.length()-1)+")";
			tv_filename.setText(file);
			convertView.setId(position);
			return convertView;
		}
	}
}