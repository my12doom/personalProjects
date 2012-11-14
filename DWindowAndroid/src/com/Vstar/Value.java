package com.Vstar;

import java.util.ArrayList;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

public  class  Value<T> {
	private final String TableName="KeyValue";
	private T mObj;
	private String mKey;
	private static ArrayList<Value> valueTable;
	private SQLiteDatabase db;
	public static Value newValue(String Key,Object Default)
	{
		int i=0;
		if(valueTable==null)
			valueTable=new ArrayList<Value>();
		int count=valueTable.size();
		for(i=0;i<count;i++)
		{
			Value v=valueTable.get(i);
			if(v.mKey.equals(Key))
			{
				return v;
			}
		}
		
		Value v=new Value(Key,Default);
		valueTable.add(v);
		return v;
	}
	private Value(String Key,T Default)
	{
		mKey=Key;
		mObj=Default;
		db=DateBase.getDb();
		if(db!=null)
		{
			Cursor cursor = null;
			try {
				db.execSQL("create table if not exists KeyValue (Key varchar(255) primary key,Value varchar(255))");
				cursor = db.query(TableName, new String[] { "Value" },
						"Key=?", new String[] { mKey }, null,null, null);
				if(cursor.moveToFirst())
				{
					//mValue=cursor.getString(0);
					
					if (mObj.getClass() == Float.class)
						mObj = (T)(Object)Float.parseFloat(cursor.getString(0));
					
					else if (mObj.getClass() == Integer.class)
						mObj = (T)(Object)Integer.parseInt(cursor.getString(0));
					
					else if (mObj.getClass() == Boolean.class)
						mObj = (T)(Object)Boolean.parseBoolean(cursor.getString(0));
					
					else
						mObj = (T)(Object)cursor.getString(0);
					
				}else
				{
					Insert(mObj.toString());
				}
			}catch (Exception e) {
				;
			} finally {
				if(cursor!=null)
				{
					cursor.close();
					cursor=null;
				}
			}
		}
	}
	
	
	public T get()
	{
		return mObj;
	}
	
	public void set(T v)
	{
		if(v!=null)
		{			
			mObj = v;
			Insert(v.toString());
		}
	}
	
	private boolean Insert(String value)
	{
		boolean res=false;
		if(db!=null)
		{
			ContentValues cv = new ContentValues();
			try {
				cv.put("Key", mKey);
				cv.put("Value", value);
				if(db.update(TableName, cv, "Key=?", new String[] { mKey })<=0){
					db.insert(TableName, null, cv);
				}
				res=true;
			}catch (Exception e) {
				;
			} finally {
				if(cv!=null)
				{
					cv.clear();
					cv=null;
				}
			}
		}
		return res;
	}
}
