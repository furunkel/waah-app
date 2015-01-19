package org.waah.example;

import java.io.IOException;

import android.app.Activity;
import android.content.Intent;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;


public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		LinearLayout layout = (LinearLayout) findViewById(R.id.layout);
        Resources res = getResources();
        AssetManager am = res.getAssets();
        String[] files;
		try {
			files = am.list("");
	        if(files != null) {   
	        	for(final String file: files) {
	        		if(file.endsWith(".rb")) {
	        			Button button = new Button(this);
	        			
	        			button.setOnClickListener(new OnClickListener() {
							@Override
							public void onClick(View v) {
								Intent intent = new Intent(MainActivity.this, ExampleActivity.class);
								intent.putExtra("file_name", file);
								MainActivity.this.startActivity(intent);

							}
						});
	        			
	        			button.setText(file.replace(".rb", ""));
	        			layout.addView(button);
	        		}
	        	}
	        }
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
