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
	private boolean showAllFiles = false;
	private boolean listBD = false;
	private ListViewAdapter adapter;
	private ListView listView;
	private DWindowNetworkConnection conn = SplashWindow3DActivity.conn;
	String[] exts = {".mp4", ".mkv", ".avi", ".rmvb", ".wmv", ".avs", ".ts", ".m2ts", ".ssif", ".mpls", ".3dv", ".e3d", ".iso"};
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
		listBD = intent.getBooleanExtra("BD", false);
		showAllFiles = intent.getBooleanExtra("showAllFiles", false);
		String [] intentSelectionRange = (String[]) intent.getSerializableExtra("selectionRange");
		if (intentSelectionRange != null)
			selectionRange = intentSelectionRange;
		String [] exts_external = (String[]) intent.getSerializableExtra("exts");
		if (exts_external != null)
			exts = exts_external;
		refresh();
		
		
		// UI init
		listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setDividerHeight(0);
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id)
        	{
        		if (listBD)
         		{
        			String drive = position >= 0 ? selectionRange[position] : "";
         			if (!drive.endsWith("/") && !drive.endsWith(":"))
         			{
 	        			result_file = drive.substring(0,3);
 	        			result_selected = true;
 	        			myFinish();
         			}
         			else
         			{
         				Toast.makeText(SelectfileActivity.this, drive.endsWith("/")?R.string.NoDisc:R.string.NonMovieDisc, Toast.LENGTH_SHORT).show();
         			}
         		}
        		else
        		{
	        		boolean isParentButton = path.length() > 1 && position == 0;
	        		if (path.length() > 1) position --;
	        		String file = position >= 0 ? selectionRange[position] : "";
	        		if (isParentButton)
	        		{
         				String oldpath = path;
        				path = path.substring(0,path.lastIndexOf("\\"));
        				path = path.substring(0,path.lastIndexOf("\\")+1);
            			if (!refresh())
            				path = oldpath;        				
	        			
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
			}
        });
        
    }
        
    private boolean isMediaFile(String filename){    	
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
		
		// remove unknown file extension if needed
		if (!showAllFiles && !listBD)
		{
			int count = 0;
			for(int i=0; i<selectionRange.length; i++)
			{
				if (isMediaFile(selectionRange[i]) || selectionRange[i].endsWith("\\"))
					count ++;				
			}
			String [] tmp = new String[count];
			for(int i=0, j=0; i<selectionRange.length; i++)
			{
				if (isMediaFile(selectionRange[i])|| selectionRange[i].endsWith("\\"))
					tmp[j++] = selectionRange[i];
			}
			selectionRange = tmp;
		}
		
		// sort
		Arrays.sort(selectionRange, new Comparator<String>()
		{
			public int compare(String lhs, String rhs)
			{
				if (lhs.endsWith("\\") != rhs.endsWith("\\"))
					return lhs.endsWith("\\") ? -1 : 1;
				
				if (isMediaFile(lhs) && isMediaFile(rhs))
					;
				else
				{
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
    	resultIntent.putExtra("path", path);
    	resultIntent.putExtra("selected", result_selected);
    	resultIntent.putExtra("selected_file", result_file);
    	
    	setResult(0, resultIntent);
    	finish();
    }
    
    private String filterBD(String c)
    {
    	if (c.endsWith("/"))
    		c = c.substring(0, c.length()-1) + getResources().getString(R.string.NoDisc);
    	else if (c.endsWith(":"))
    		c = c.substring(0, c.length()-1) + getResources().getString(R.string.NonMovieDisc);
    	return c;
    }
    
    private class ListViewAdapter extends BaseAdapter {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) {
			inflater = LayoutInflater.from(context);
		}

		public int getCount() {
    		boolean addParentButton = path.length() > 1 && !listBD;
			return selectionRange.length + (addParentButton ? 1 : 0);
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
	    	boolean addParentButton = path.length() > 1 && !listBD;
	    	String file;
	    	if (addParentButton)
	    		file = position == 0 ? getResources().getString(R.string.GoToParent) : selectionRange[position-1];
	    	else
	    		file = selectionRange[position];
	    		
			if (listBD)		// BD drivers
				file = filterBD(file);
			else if (file.endsWith("\\"))	// files and folders
			{
				file = "("+file.substring(0, file.length()-1)+")";
			}
			tv_filename.setText(file);
			convertView.setId(position);
			return convertView;
		}
	}
    
    public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			myFinish();			
		}
		return false;
    }    
}
