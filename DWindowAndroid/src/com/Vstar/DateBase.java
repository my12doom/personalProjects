package com.Vstar;

import java.io.File;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;

public class DateBase {
	private static  DateBase dbs;
	private Context mContext=null;
	private SQLiteDatabase db=null;  
	private final String DB_NAME = "3dapp";
	
	/**
	 * 实例化本类
	 */
	public static void Init(Context context)
	{
		newDateBase().mContext=context;
	}
	
	/**
	 * 获取操作数据库对象的实例
	 * @return
	 */
	public static SQLiteDatabase getDb()
	{
		return newDateBase().getDatabase();
	}
	
	/**
	 * 获取本类的实例
	 */
	private static DateBase newDateBase()
	{
		if(dbs!=null)
			return dbs;
		dbs=new DateBase();
		return dbs;
	}
	
	/**
	 * 获取数据库实例
	 * @return
	 */
	private SQLiteDatabase getDatabase()
	{
		if (db==null)
			if(mContext!=null)
			{
				String path=mContext.getDatabasePath(DB_NAME).getParent();
				File destDir = new File(path);
				 if (!destDir.exists()) {
					   destDir.mkdirs();
				  }
				 if(destDir.exists())
				 {
					 db=SQLiteDatabase.openOrCreateDatabase(path+"/"+DB_NAME, null);
				 }
			}
		return db;
	}
	
	
}
