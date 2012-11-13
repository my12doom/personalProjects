package com.demo.splash;


import java.util.Arrays;
import java.util.Comparator;

import com.Vstar.Splash;
import com.Vstar.Splash.OnSplashCompletionListener;
import com.demo.splash.DWindowNetworkConnection.cmd_result;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

public class SplashWindow3DActivity extends Activity {
	
	String[] file_list = new String[]{"connection failed"};
	BDROMEntry[] bdrom_entry;
	ListViewAdapter adapter;
	ListView listView;
	DWindowNetworkConnection conn = new DWindowNetworkConnection();
	
	private class BDROMEntry{
		String path;
		String text;
		int state;	// 0 = no disc, 1 = non-movie disc, 2 = bd
	}
	
	private void connect()
	{
		conn.connect("192.168.10.199");
		int login_result = conn.login("TestCode");
		file_list = new String[]{login_result == 1 ? "Login OK" : (login_result == 0 ? "Password Error" : "Login Failed")};
		adapter.notifyDataSetChanged();
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
    			if (conn.getState(true)<0)
    				connect();
        		if (!file.endsWith("\\")){
        			conn.execute_command("reset_and_loadfile|"+path.substring(1,path.length())+file);
        		}
        		else
        		{
         			if (path.equals("\\") && isBDPath(file))
         				conn.execute_command("reset_and_loadfile|"+file);
         			else{
         				String oldpath = path;
	         			path += file;
	        			if (!refresh())
	        				path = oldpath;
	        			
	        			listView.setSelection(0);
         			}
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
				connect();
				refresh();				
				listView.setSelection(0);
			}
		});
	}
    
    private boolean isMediaFile(String filename){
    	String[] exts = {".mp4", ".mkv", ".avi", ".rmvb", ".wmv", ".avs", ".ts", ".m2ts", ".ssif", ".mpls", ".3dv", ".e3d"};
    	
    	for(int i=0; i<exts.length; i++)
    		if (filename.toLowerCase().endsWith(exts[i]))
    			return true;
    	return false;
    }
    
    private String C(String c){
    	return c;
    }
    
    private boolean isBDPath(String path){
    	for(int i=0; i<bdrom_entry.length; i++){
    		if (bdrom_entry[i].path.equalsIgnoreCase(path) && bdrom_entry[i].state == 2)
    			return true;    			
    	}
    	return false;
    }
    
    private String path = "\\";
    private boolean refresh(){
    	// reconnect if
		if (conn.getState()<0)
			connect();
    	
    	// file list
		String cmd = path.equals("\\") ? "list_drive" : "list_file|" + path.substring(1, path.length());
		cmd_result list_result = conn.execute_command(cmd);
		if (list_result.successed())
			file_list = list_result.result.split("\\|");
		
		// sort
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
		
		// bd list
		if (path.equals("\\")){
			list_result = conn.execute_command("list_bd3d");
			if (list_result.successed())
			{
				String[] bd_list = list_result.result.split("\\|");
				bdrom_entry = new BDROMEntry[bd_list.length];
				for(int i=0; i<bdrom_entry.length; i++)
				{
					bdrom_entry[i] = new BDROMEntry();
					bdrom_entry[i].path = bd_list[i].substring(0,3);
					bdrom_entry[i].text = bd_list[i].substring(3,bd_list[i].length());
					
					if (bdrom_entry[i].text.equalsIgnoreCase("/"))
						bdrom_entry[i].state = 0;
					else if (bdrom_entry[i].text.equalsIgnoreCase(":"))
						bdrom_entry[i].state = 1;
					else
						bdrom_entry[i].state = 2;
				}
			}
		}

		// refresh
		adapter.notifyDataSetChanged();
		
		return conn.getState() >= 0;
    }
    
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (conn.getState(true)<0)
				connect();
			if (path.length() > 1){
 				String oldpath = path;
				path = path.substring(0,path.lastIndexOf("\\"));
				path = path.substring(0,path.lastIndexOf("\\")+1);
    			if (!refresh())
    				path = oldpath;
				
				return true;
			}
			else{
				finish();
			}
		} else if (keyCode == KeyEvent.KEYCODE_MENU){
			//refresh();
			long l = System.currentTimeMillis();
			byte[] jpg = conn.shot();
			l = System.currentTimeMillis() - l;
			System.out.println("speed:" + (l>0?jpg.length / l : -1));
			l = System.currentTimeMillis();
			if (jpg != null)
			{
				Bitmap bmp = BitmapFactory.decodeByteArray(jpg, 0, jpg.length);
				BitmapDrawable bd = new BitmapDrawable(bmp);
				LinearLayout layout = (LinearLayout)findViewById(R.id.main_layout);
				layout.setBackgroundDrawable(bd);
				l = System.currentTimeMillis() - l;
				System.out.println("speed:" + (l>0?jpg.length / l : -1));
			}
		}
		return false;
    }    
        
    private class ListViewAdapter extends BaseAdapter {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) {
			inflater = LayoutInflater.from(context);
		}

		public int getCount() {
			return file_list.length;
		}

		public Object getItem(int position) {
			return null;
		}

		public long getItemId(int position) {
			return 0;
		}

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