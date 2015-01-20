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
	
	public static String buffer = "";
	
	static public boolean setVisible(boolean visible)
	{
		final Activity activity = WaahActivity.getInstance();
        InputMethodManager imm = (InputMethodManager) WaahActivity.getInstance()
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
		if(c == '\b' || c == '\n') return true;
		
	    Character.UnicodeBlock block = Character.UnicodeBlock.of(c);
	    return (!Character.isISOControl(c)) &&
	            block != null &&
	            block != Character.UnicodeBlock.SPECIALS;
	}
	
	static public boolean dispatchKeyEvent(KeyEvent event) {
		
		switch(event.getAction()) {
			case KeyEvent.ACTION_MULTIPLE:
				if(event.getKeyCode() == KeyEvent.KEYCODE_UNKNOWN) {
					buffer = event.getCharacters();
				} else {
					buffer = "";
				}
				break;
			default:
				if(event.getKeyCode() == KeyEvent.KEYCODE_DEL) {
					buffer = "\b";
				} else {
					buffer = new String(Character.toChars(event.getUnicodeChar()));
				}
				break;
		}
		if(buffer == null) {
			buffer = "";
		}
		
		Log.i("waah", String.format("Str <%s>", buffer));
		
		int firstCodePoint = buffer.codePointAt(0);
		if(!isPrintableChar(firstCodePoint)) {
			Log.i("waah", "not printable");
			buffer = "";
		}
		
		return false;
	}
	
	static public String getBuffer() {
		return buffer;
	}
}
