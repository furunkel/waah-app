package org.waah;

import java.io.UnsupportedEncodingException;

import android.app.Activity;
import android.app.NativeActivity;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.util.Log;

public class WaahActivity extends NativeActivity {
	private static WaahActivity instance = null;
	private static final String FILE_NAME_KEY = "org.waah.file_name";
	private static final String DEFAULT_FILE_NAME= "app.rb";
	private String fileName = null;
	
    static {
        System.loadLibrary("waah");
    }
	
    public static Activity getInstance() {
    	return instance;
    }
    
    public void setFileName(String fileName) {
    	this.fileName = fileName;
    }
    
    public byte[] getFileName() {
    	try {
			return fileName.getBytes("UTF-8");
		} catch (UnsupportedEncodingException e) {
			return new byte[0];
		}
    }
    
    @Override
    public boolean dispatchKeyEvent(android.view.KeyEvent event) {
    	Keyboard.dispatchKeyEvent(event);
    	return super.dispatchKeyEvent(event);
    };
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		instance = this;
		
		if(fileName == null) {
			ActivityInfo ai;
			try {
				ai = getPackageManager().getActivityInfo(getIntent().getComponent(), PackageManager.GET_META_DATA);
				if(ai.metaData != null) {
					fileName = ai.metaData.getString(FILE_NAME_KEY);
				}
			} catch (NameNotFoundException e) {
			}
		}
		
		if(fileName == null || fileName.isEmpty()) {
			fileName = DEFAULT_FILE_NAME;
		}
		
		super.onCreate(savedInstanceState);
	}
}
