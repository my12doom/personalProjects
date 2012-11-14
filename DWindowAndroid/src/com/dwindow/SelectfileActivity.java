package com.dwindow;

import java.util.Arrays;
import java.util.Comparator;

import com.demo.splash.R;
import com.dwindow.DWindowNetworkConnection.cmd_result;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

public class SelectfileActivity extends Activity 
{
	private Intent resultIntent = new Intent();
	private String[] selectionRange;
	private String path;
	private ListViewAdapter adapter;
	private ListView listView;
	private DWindowNetworkConnection conn = SplashWindow3DActivity.conn;

    public void onCreate(Bundle savedInstanceState) 
    {
		super.onCreate(savedInstanceState);
		
		// intents
		Intent intent = getIntent();
		path = intent.getStringExtra("path");
		selectionRange = (String[]) intent.getSerializableExtra("selectionRange");
		
		
		// UI init
		listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener(){
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
        		if (position==0)
        			return;
        		String file = selectionRange[position-1];
    			if (conn.getState(true)<0)
    				return;
        		if (!file.endsWith("\\"))
        		{
        		}
        		else
        		{
         				String oldpath = path;
	         			path += file;
	        			if (!refresh())
	        				path = oldpath;
	        			
	        			listView.setSelection(0);
        		}
			}
        });
        
        listView.setDividerHeight(0);
		setContentView(R.layout.selectfile);
    }
    
    private void connect()
	{
		conn.connect("192.168.1.199");
		int login_result = conn.login("TestCode");
		selectionRange = new String[]{login_result == 1 ? "Login OK" : (login_result == 0 ? "Password Error" : "Login Failed")};
		adapter.notifyDataSetChanged();
	}
    
    private boolean isMediaFile(String filename){
    	String[] exts = {".mp4", ".mkv", ".avi", ".rmvb", ".wmv", ".avs", ".ts", ".m2ts", ".ssif", ".mpls", ".3dv", ".e3d"};
    	
    	for(int i=0; i<exts.length; i++)
    		if (filename.toLowerCase().endsWith(exts[i]))
    			return true;
    	return false;
    }
    
    private boolean refresh(){
    	// reconnect if
		if (conn.getState()<0)
			connect();
    	
    	// file list
		String cmd = path.equals("\\") ? "list_drive" : "list_file|" + path.substring(1, path.length());
		cmd_result list_result = conn.execute_command(cmd);
		if (list_result.successed())
			selectionRange = list_result.result.split("\\|");
		
		// sort
		Arrays.sort(selectionRange, new Comparator<String>(){
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

		// refresh
		adapter.notifyDataSetChanged();
		
		return conn.getState() >= 0;
    }
    
    public void onStop()
    {
    	resultIntent.putExtra("Hello", "World");
    	setResult(0, resultIntent);
    }
    

    private class ListViewAdapter extends BaseAdapter {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) {
			inflater = LayoutInflater.from(context);
		}

		public int getCount() {
			return selectionRange.length;
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
			String file = selectionRange[position];
			if (file.endsWith("\\"))
				file = "("+file.substring(0, file.length()-1)+")";
			tv_filename.setText(file);
			convertView.setId(position);
			return convertView;
		}
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
}
