/*
 * Copyright (c) 2018 Taner Sener
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg.test;

import android.arch.core.util.Function;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.Toast;

import com.arthenica.mobileffmpeg.FFmpeg;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "mobile-ffmpeg-test";

    private EditText commandText;
    private EditText statusText;
    private String asyncStatus;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        commandText = findViewById(R.id.commandText);
        statusText = findViewById(R.id.statusText);
        asyncStatus = null;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        FFmpeg.shutdown();
    }

    public void runFFmpeg(View view) {
        String command = commandText.getText().toString();
        String[] split = command.split(" ");
        int returnCode = FFmpeg.execute(split);
        setStatus((returnCode == 0)?"Success":"Process completed with rc=" + returnCode);
    }

    public void runFFmpegAsync(View view) {
        String command = commandText.getText().toString();
        String[] arguments = command.split(" ");

        setStatus("Running");
        asyncStatus = null;

        final Handler handler = new Handler();
        final Runnable runnable = new Runnable() {

            @Override
            public void run() {
                if (asyncStatus != null) {
                    setStatus(asyncStatus);
                    handler.removeCallbacks(this);
                }
            }
        };
        handler.postDelayed(runnable, 1000);

        FFmpeg.executeAsync(new Function<Integer, Void>() {

            @Override
            public Void apply(Integer returnCode) {
                asyncStatus = (returnCode == 0)?"Success":"Async process completed with rc=" + returnCode;
                return null;
            }
        }, arguments);
    }

    public void setStatus(final String result) {
        statusText.setText(result);
    }

    public void createSlideshow(View view) {
        final String image1 = "colosseum.jpg";
        final String image2 = "pyramid.jpg";
        final String image3 = "tajmahal.jpg";
        final String video = "slideshow.mp4";

        setStatus("Running");
        asyncStatus = null;

        final Handler handler = new Handler();
        final Runnable runnable = new Runnable() {

            @Override
            public void run() {
                if (asyncStatus != null) {
                    setStatus(asyncStatus);
                    handler.removeCallbacks(this);
                }
            }
        };
        handler.postDelayed(runnable, 1000);

        try {
            resourceToFile(R.drawable.colosseum, image1);
            resourceToFile(R.drawable.pyramid, image2);
            resourceToFile(R.drawable.tajmahal, image3);

            File file = new File(getFilesDir(), video);
            if (file.exists()) {
                file.delete();
            }

            String script = Slideshow.generate(getFilesDir(), image1, image2, image3, video);
            Log.d(TAG, script);
            FFmpeg.executeAsync(new Function<Integer, Void>() {

                @Override
                public Void apply(Integer returnCode) {
                    asyncStatus = (returnCode == 0)?"Success":"Async process completed with rc=" + returnCode;
                    return null;
                }
            }, script.split(" "));
        } catch (IOException e) {
            Log.e(TAG, "Creating slideshow failed", e);
            Toast.makeText(this, "", Toast.LENGTH_SHORT).show();
        }
    }

    protected void resourceToFile(final int resourceId, final String fileName) throws IOException {
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), resourceId);

        File file = new File(getFilesDir(), fileName);
        if (file.exists()) {
            file.delete();
        }

        FileOutputStream outputStream = new FileOutputStream(file);
        bitmap.compress(Bitmap.CompressFormat.JPEG, 90, outputStream);
        outputStream.flush();
        outputStream.close();
        Log.d(TAG, String.format("Saved resource %d to file %s.", resourceId, fileName));
    }

}
