package com.dwindow;

import java.util.Arrays;
import java.util.Comparator;

import com.Vstar.DateBase;
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
import android.widget.Toast;

public class SelectfileActivity extends Activity 
{
	private Intent resultIntent = new Intent();
	private String[] selectionRange;
	private String path;
	private boolean listBD = false;
	private ListViewAdapter adapter;
	private ListView listView;
	private DWindowNetworkConnection conn = SplashWindow3DActivity.conn;
	
	private boolean result_selected;
	private String result_file;

    public void onCreate(Bundle savedInstanceState) 
    {
    	// basic
		super.onCreate(savedInstanceState);
		setContentView(R.layout.selectfile);		
		DateBase.Init(this);
		
		// intents
		Intent intent = getIntent();
		path = intent.getStringExtra("path");
		String [] intentSelectionRange = (String[]) intent.getSerializableExtra("selectionRange");
		listBD = intent.getBooleanExtra("BD", false);
		if (intentSelectionRange != null)
			selectionRange = intentSelectionRange;
		refresh();
		
		
		// UI init
		listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id)
        	{
        		String file = selectionRange[position];
        		if (listBD)
         		{
         			if (!file.endsWith("/") && !file.endsWith(":"))
         			{
 	        			result_file = file.substring(0,3);
 	        			result_selected = true;
 	        			myFinish();
         			}
         			else
         			{
         				Toast.makeText(SelectfileActivity.this, file.endsWith("/")?"No Disc":"Non Movie Disc", Toast.LENGTH_SHORT).show();
         			}
         		}
        		else if (!file.endsWith("\\"))
        		{
        			result_file = path.substring(1, path.length()) + file;
        			result_selected = true;
        			myFinish();
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
    }
        
    private boolean isMediaFile(String filename){
    	String[] exts = {".mp4", ".mkv", ".avi", ".rmvb", ".wmv", ".avs", ".ts", ".m2ts", ".ssif", ".mpls", ".3dv", ".e3d"};
    	
    	for(int i=0; i<exts.length; i++)
    		if (filename.toLowerCase().endsWith(exts[i]))
    			return true;
    	return false;
    }
    
    private boolean refresh(){    	
    	// file list
		String cmd = path.equals("\\") ? "list_drive" : "list_file|" + path.substring(1, path.length());
		cmd = listBD ? "list_bd" : cmd;
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
		if (adapter != null)
			adapter.notifyDataSetChanged();
		
		return conn.getState() >= 0;
    }
    
    private void myFinish()
    {
    	resultIntent.putExtra("Hello", "World");
    	resultIntent.putExtra("selected", result_selected);
    	resultIntent.putExtra("selected_file", result_file);
    	
    	setResult(0, resultIntent);
    	finish();
    }
    
    private String filterBD(String c)
    {
    	if (c.endsWith("/"))
    		c = c.substring(0, c.length()-1) + "(No Disc)";
    	else if (c.endsWith(":"))
    		c = c.substring(0, c.length()-1) + "(Non Movie Disc)";
    	return c;
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
			if (listBD)		// BD drivers
				file = filterBD(file);
			else if (file.endsWith("\\"))	// files and folders
				file = "("+file.substring(0, file.length()-1)+")";
			tv_filename.setText(file);
			convertView.setId(position);
			return convertView;
		}
	}
    
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			if (listBD)
				myFinish();
			else if (path.length() > 1){
 				String oldpath = path;
				path = path.substring(0,path.lastIndexOf("\\"));
				path = path.substring(0,path.lastIndexOf("\\")+1);
    			if (!refresh())
    				path = oldpath;
				
				return true;
			}
			else{
				myFinish();
			}
		}
		return false;
    }    
}
