package com.dwindow.util;

import java.io.File;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;

public class DateBase {
	private static DateBase dbs;
	private Context mContext = null;
	private SQLiteDatabase db = null;
	private final String DB_NAME = "3dapp";

	public static void Init(Context context) {
		newDateBase().mContext = context;
	}

	public static SQLiteDatabase getDb() {
		return newDateBase().getDatabase();
	}

	private static DateBase newDateBase() {
		if (dbs != null)
			return dbs;
		dbs = new DateBase();
		return dbs;
	}

	private SQLiteDatabase getDatabase() {
		if (db == null)
			if (mContext != null) {
				String path = mContext.getDatabasePath(DB_NAME).getParent();
				File destDir = new File(path);
				if (!destDir.exists()) {
					destDir.mkdirs();
				}
				if (destDir.exists()) {
					db = SQLiteDatabase.openOrCreateDatabase(path + "/"
							+ DB_NAME, null);
				}
			}
		return db;
	}
}
