package com.hhbgk.h264tojpg;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import com.hhbgk.h264tojpg.api.H264ToJpg;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private ImageView mImageView;
    private H264ToJpg mH264ToJpg;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (mH264ToJpg == null)
            mH264ToJpg = new H264ToJpg();

        mImageView = (ImageView) findViewById(R.id.show_picture);
        Button button = (Button) findViewById(R.id.show_btn);
        assert button != null;
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String h264 = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getPath() + "/i.h264";
                String jpg = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getPath() + "/new.jpeg";
                if (new File(h264).exists()){
                    mH264ToJpg.requestJpg(h264, jpg);
                } else {
                    Toast.makeText(MainActivity.this, h264 + " not exist", Toast.LENGTH_LONG).show();
                }
            }
        });

        mH264ToJpg.setOnDataPreparedListener(new H264ToJpg.OnDataPreparedListener() {
            @Override
            public void onPrepared(byte[] data) {
                Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
                mImageView.setImageBitmap(bitmap);
            }
        });
    }
}
