package org.waah.example;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;


public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
	}
	
	public void startExample1(View button) {
		Intent intent = new Intent(MainActivity.this, ExampleActivity.class);
		MainActivity.this.startActivity(intent);
	}
}
