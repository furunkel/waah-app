package org.waah.example;

import org.waah.WaahActivity;

import android.os.Bundle;
import android.util.Log;

public class ExampleActivity extends WaahActivity {
	@Override
	protected void onCreate(Bundle savedInstanceState) {

		setFileName(this.getIntent().getStringExtra("file_name"));
		Log.i("waah", "file_name=" + new String(getFileName()));
		
		super.onCreate(savedInstanceState);
	}
	
}
