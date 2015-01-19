package org.waah.example;

import org.waah.WaahActivity;

import android.os.Bundle;

public class ExampleActivity extends WaahActivity {

	static {
		java.lang.System.loadLibrary("waah");
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	}
}
