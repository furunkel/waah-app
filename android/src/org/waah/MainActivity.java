package org.waah;

import android.app.Activity;
import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends NativeActivity {
	private static MainActivity instance = null;
	
    static {
        System.loadLibrary("waah");
     }
	
    public static Activity getInstance() {
    	return instance;
    }
    
    @Override
    public boolean dispatchKeyEvent(android.view.KeyEvent event) {
    	Keyboard.dispatchKeyEvent(event);
    	return super.dispatchKeyEvent(event);
    };
    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		instance = this;
		
		super.onCreate(savedInstanceState);
	}
	
}
