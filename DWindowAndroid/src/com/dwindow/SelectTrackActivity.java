// this class is very similar to select file activity

package com.dwindow;


import com.Vstar.DateBase;
import com.demo.splash.R;
import com.dwindow.DWindowNetworkConnection.cmd_result;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class SelectTrackActivity extends Activity 
{
	private Intent resultIntent = new Intent();
	private String[] selectionRange = new String[]{"No Track", "False"};
	boolean hasTrack = false;
	private String track_source;
	private ListViewAdapter adapter;
	private ListView listView;
	private DWindowNetworkConnection conn = SplashWindow3DActivity.conn;
	
	private boolean result_selected;
	private int result_track;
	private final int request_code_loadsubtitle = 20000;

    public void onCreate(Bundle savedInstanceState) 
    {
    	// basic
		super.onCreate(savedInstanceState);
		setContentView(R.layout.selectfile);		
		DateBase.Init(this);
		
		// intents
		Intent intent = getIntent();
		track_source = intent.getStringExtra("track");
		
		// load track list
		refresh();		
		
		// UI init
		listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id)
        	{
        		// "Load Subtitle"
        		if (position >= selectionRange.length/2)
        		{
        			Intent intent = new Intent(SelectTrackActivity.this, SelectfileActivity.class);
    				intent.putExtra("path", SplashWindow3DActivity.path.get());
    				intent.putExtra("exts", new String[]{".srt", ".ssa", ".ass", ".ass2", ".ssa", ".sup", ".idx", ".sub"});
    				startActivityForResult(intent, request_code_loadsubtitle);
        		}
        		
        		if (!hasTrack)
        			return;
        		
        		conn.execute_command("enable_" + track_source +"_track|"+position);
        		refresh();
			}
        });
        
        listView.setDividerHeight(0);
        listView.setAdapter(adapter = new ListViewAdapter(this));
    }
    
    private void refresh()
    {
		cmd_result result = conn.execute_command("list_"+track_source+"_track");
		if (result.successed())
		{
			selectionRange = result.result.split("\\|");
			hasTrack = true;
		}
		if (adapter != null)
			adapter.notifyDataSetChanged();
    }
    
    private void myFinish()
    {
    	resultIntent.putExtra("selected", result_selected);
    	resultIntent.putExtra("selected_track", result_track);
    	
    	setResult(0, resultIntent);
    	finish();
    }
        
    private class ListViewAdapter extends BaseAdapter 
    {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) 
		{
			inflater = LayoutInflater.from(context);
		}

		public int getCount() 
		{
			return selectionRange.length/2+(track_source.equals("subtitle") ? 1 : 0);
		}

		public Object getItem(int position) 
		{
			return null;
		}

		public long getItemId(int position) 
		{
			return 0;
		}

		public View getView(int position, View convertView, ViewGroup parent) 
		{
			if (convertView == null)
				convertView = inflater.inflate(R.layout.lv_track_item, null);
			
			TextView tv_filename = (TextView) convertView.findViewById(R.id.tv_open_filename);
			ImageView iv_selected = (ImageView) convertView.findViewById(R.id.iv_selected);
			String file = (position >= selectionRange.length/2) ? "Load Subtitle..." : selectionRange[position*2];
			
			boolean selected = false;
			try
			{
				selected = Boolean.parseBoolean(selectionRange[position*2+1]);				
			}catch(Exception e){};			
			tv_filename.setText(file);
			if (!selected)
				iv_selected.setImageBitmap(null);
			else
				iv_selected.setImageResource(R.drawable.selected);
			
			convertView.setId(position);
			return convertView;
		}
	}
    
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
    	if (requestCode == request_code_loadsubtitle)
    	{
    		boolean selected = data.getBooleanExtra("selected", false);
    		String selected_file = data.getStringExtra("selected_file");
    		SplashWindow3DActivity.path.set(data.getStringExtra("path"));
    		
    		if (selected)
    		{
    			conn.execute_command("load_subtitle|"+selected_file);
    			refresh();
    		}
    	}
    }
    
    public boolean onKeyDown(int keyCode, KeyEvent event) 
    {
		if (keyCode == KeyEvent.KEYCODE_BACK) 
		{
			myFinish();
		}
		return false;
    }    
}
