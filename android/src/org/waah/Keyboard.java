package org.waah;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;

import android.app.Activity;
import android.content.Context;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

public class Keyboard {
	
	public static byte[] buffer = new byte[0];
	
	static public boolean setVisible(boolean visible)
	{
		final Activity activity = MainActivity.getInstance();
        InputMethodManager imm = (InputMethodManager) MainActivity.getInstance()
                .getSystemService(Context.INPUT_METHOD_SERVICE);
        if(imm != null) {
        	View view = activity.getWindow().getDecorView();
        	if(visible) {
        		imm.showSoftInput(view, 0);
        	} else {
	        	imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        	}
        	return true;
        }
        return false;
	}
	
	// Adapted version
	// of http://stackoverflow.com/questions/220547/printable-char-in-java
	static public boolean isPrintableChar(int c ) {
	    Character.UnicodeBlock block = Character.UnicodeBlock.of(c);
	    return (!Character.isISOControl(c)) &&
	            block != null &&
	            block != Character.UnicodeBlock.SPECIALS;
	}
	
	static public boolean dispatchKeyEvent(KeyEvent event) {
		String str;
		
		switch(event.getAction()) {
			case KeyEvent.ACTION_MULTIPLE:
				if(event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
					str = event.getCharacters();
				} else {
					str = "";
				}
				break;
			default:
				str = new String(Character.toChars(event.getUnicodeChar()));		
				break;
		}
		if(str == null) {
			str = "";
		}
		
		int firstCodePoint = str.codePointAt(0);
		if(!isPrintableChar(firstCodePoint)) {
			str = "";
		}
		
		try {
			buffer = str.getBytes("UTF-8");
		} catch (UnsupportedEncodingException e) {
			buffer = new byte[0];
		}
		
		return false;
	}
	
	static public byte[] getBuffer() {
		return buffer;
	}
}
