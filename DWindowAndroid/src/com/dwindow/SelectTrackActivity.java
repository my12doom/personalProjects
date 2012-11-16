// this class is very similar to select file activity

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
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class SelectTrackActivity extends Activity 
{
	private Intent resultIntent = new Intent();
	private String[] selectionRange = new String[]{"No Track", "False"};
	boolean hasTrack = false;
	private String track_source;
	private boolean listBD = false;
	private ListViewAdapter adapter;
	private ListView listView;
	private DWindowNetworkConnection conn = SplashWindow3DActivity.conn;
	
	private boolean result_selected;
	private int result_track;

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
		cmd_result result = conn.execute_command("list_"+track_source+"_track");
		if (result.successed())
		{
			selectionRange = result.result.split("\\|");
			hasTrack = true;
		}
		
		
		// UI init
		listView = (ListView)findViewById(R.id.lv_player_list);
        listView.setAdapter(adapter = new ListViewAdapter(this));
        listView.setOnItemClickListener(new AdapterView.OnItemClickListener()
        {
        	public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id)
        	{
        		if (!hasTrack)
        			return;
        		
        		result_selected = true;
        		result_track = position;
        		myFinish();
			}
        });
        
        listView.setDividerHeight(0);
        listView.setAdapter(adapter = new ListViewAdapter(this));
    }
    
    
    private void myFinish()
    {
    	resultIntent.putExtra("selected", result_selected);
    	resultIntent.putExtra("selected_track", result_track);
    	
    	setResult(0, resultIntent);
    	finish();
    }
        
    private class ListViewAdapter extends BaseAdapter {
		LayoutInflater inflater;

		public ListViewAdapter(Context context) {
			inflater = LayoutInflater.from(context);
		}

		public int getCount() {
			return selectionRange.length/2;
		}

		public Object getItem(int position) {
			return null;
		}

		public long getItemId(int position) {
			return 0;
		}

		public View getView(int position, View convertView, ViewGroup parent) {
			if (convertView == null)
				convertView = inflater.inflate(R.layout.lv_track_item, null);
			TextView tv_filename = (TextView) convertView.findViewById(R.id.tv_open_filename);
			ImageView iv_selected = (ImageView) convertView.findViewById(R.id.iv_selected);
			String file = selectionRange[position*2];
			boolean selected = false;
			try
			{
				selected = Boolean.parseBoolean(selectionRange[position*2+1]);				
			}catch(Exception e){};			
			tv_filename.setText(file);
			if (!selected)
				iv_selected.setImageBitmap(null);
			
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
