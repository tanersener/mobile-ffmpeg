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

import android.Manifest;
import android.app.ActionBar;
import android.app.AlertDialog;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActivityCompat;
import android.support.v4.view.PagerTabStrip;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.util.RunCallback;
import com.arthenica.mobileffmpeg.util.AsynchronousTaskService;
import com.arthenica.mobileffmpeg.util.RunCallback;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Queue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Future;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "mobile-ffmpeg-test";

    public static final int REQUEST_EXTERNAL_STORAGE = 1;
    public static String[] PERMISSIONS_ALL = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA
    };

    protected static final AsynchronousTaskService asynchronousTaskService = new AsynchronousTaskService();

    protected static final Queue<Callable> actionQueue = new ConcurrentLinkedQueue<>();

    protected static final Handler handler = new Handler();

    protected static final Runnable runnable = new Runnable() {

        @Override
        public void run() {
            Callable callable;

            do {
                callable = actionQueue.poll();
                if (callable != null) {
                    try {
                        callable.call();
                    } catch (final Exception e) {
                        android.util.Log.e(MainActivity.TAG, "Running UI action received error.", e);
                    }
                }
            } while (callable != null);

            handler.postDelayed(this, 250);
        }
    };

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        android.support.v7.app.ActionBar supportActionBar = getSupportActionBar();
        if (supportActionBar != null) {
            supportActionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM);
            supportActionBar.setCustomView(R.layout.action_bar);
        }

        PagerTabStrip pagerTabStrip = findViewById(R.id.pagerTabStrip);
        if (pagerTabStrip != null) {
            pagerTabStrip.setDrawFullUnderline(false);
            pagerTabStrip.setTabIndicatorColorResource(R.color.navigationColor);
            pagerTabStrip.setTextColor(Color.parseColor("#f39c12"));
        }

        final ViewPager viewPager = findViewById(R.id.pager);
        viewPager.setAdapter(new PagerAdapter(getSupportFragmentManager(), this, 6));

        waitForUIAction();

        // VERIFY PERMISSIONS
        int permission = ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    PERMISSIONS_ALL,
                    REQUEST_EXTERNAL_STORAGE);
        }
        permission = ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    PERMISSIONS_ALL,
                    REQUEST_EXTERNAL_STORAGE);
        }
        permission = ActivityCompat.checkSelfPermission(this, Manifest.permission.CAMERA);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    PERMISSIONS_ALL,
                    REQUEST_EXTERNAL_STORAGE);
        }

        try {
            registerAppFont();
            Log.d(TAG, "Application fonts registered.");
        } catch (final IOException e) {
            Log.e(TAG, "Font registration failed.", e);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        handler.removeCallbacks(runnable);
    }

    /**
     * <p>Starts a new asynchronous FFmpeg operation with arguments provided.
     *
     * @param runCallback callback function to receive result of this execution
     * @param arguments   FFmpeg command options/arguments
     * @return <code>Future</code> instance of asynchronous operation started
     */
    public static Future executeAsync(final RunCallback runCallback, final String arguments) {
        return asynchronousTaskService.runAsynchronously(new Callable<Integer>() {

            @Override
            public Integer call() {
                int returnCode = FFmpeg.execute(arguments, " ");
                if (runCallback != null) {
                    runCallback.apply(returnCode);
                }

                return returnCode;
            }
        });
    }

    public static void waitForUIAction() {
        handler.postDelayed(runnable, 250);
    }

    public static void addUIAction(final Callable callable) {
        actionQueue.add(callable);
    }

    public AlertDialog createProgressDialog(final String text) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (inflater != null) {
            View dialogView = inflater.inflate(R.layout.progress_dialog_layout, null);
            builder.setView(dialogView);

            TextView textView = dialogView.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(text);
            }
        }
        builder.setCancelable(false);
        return builder.create();
    }

    public AlertDialog createCancellableProgressDialog(final String text, final View.OnClickListener onClickListener) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (inflater != null) {
            View dialogView = inflater.inflate(R.layout.cancellable_progress_dialog_layout, null);
            builder.setView(dialogView);

            TextView textView = dialogView.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(text);
            }
            Button cancelButton = dialogView.findViewById(R.id.cancelButton);
            if (cancelButton != null) {
                cancelButton.setOnClickListener(onClickListener);
            }
        }
        builder.setCancelable(false);
        return builder.create();
    }

    protected void resourceToFile(final int resourceId, final File file) throws IOException {
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), resourceId);

        if (file.exists()) {
            file.delete();
        }

        FileOutputStream outputStream = new FileOutputStream(file);
        bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
        outputStream.flush();
        outputStream.close();
    }

    protected void rawResourceToFile(final int resourceId, final File file) throws IOException {
        final InputStream inputStream = getResources().openRawResource(resourceId);
        if (file.exists()) {
            file.delete();
        }
        final FileOutputStream outputStream = new FileOutputStream(file);

        try {
            final byte[] buffer = new byte[1024];
            int readSize;

            while ((readSize = inputStream.read(buffer)) > 0) {
                outputStream.write(buffer, 0, readSize);
            }
        } catch (final IOException e) {
            Log.e(TAG, "Saving raw resource failed.", e);
        } finally {
            inputStream.close();
            outputStream.flush();
            outputStream.close();
        }
    }

    protected void registerAppFont() throws IOException {
        final File cacheDirectory = getCacheDir();

        // SAVE FONTS
        rawResourceToFile(R.raw.doppioone_regular, new File(cacheDirectory, "doppioone_regular.ttf"));
        rawResourceToFile(R.raw.truenorg, new File(cacheDirectory, "truenorg.otf"));

        final HashMap<String, String> fontNameMapping = new HashMap<>();
        fontNameMapping.put("MyFontName", "Doppio One");
        Config.setFontDirectory(this, cacheDirectory.getAbsolutePath(), fontNameMapping);
        // Config.setFontDirectory(this, cacheDirectory.getAbsolutePath(), null);
    }

}
