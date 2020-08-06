/*
 * Copyright (c) 2018-2019 Taner Sener
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
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.Switch;
import android.widget.ToggleButton;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.viewpager.widget.PagerTabStrip;
import androidx.viewpager.widget.ViewPager;

import com.arthenica.mobileffmpeg.AsyncFFmpegExecuteTask;
import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.arthenica.mobileffmpeg.Level;
import com.arthenica.mobileffmpeg.Signal;
import com.arthenica.mobileffmpeg.util.ResourcesUtil;
import com.arthenica.smartexception.java.Exceptions;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "mobile-ffmpeg-test";

    public static final int REQUEST_EXTERNAL_STORAGE = 1;
    public static String[] PERMISSIONS_ALL = {
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA
    };

    static {
        Exceptions.registerRootPackage("com.arthenica");
    }

    protected static final Queue<Callable<Object>> actionQueue = new ConcurrentLinkedQueue<>();

    protected static final Handler handler = new Handler();

    protected static final Runnable runnable = new Runnable() {

        @Override
        public void run() {
            Callable<Object> callable;

            do {
                callable = actionQueue.poll();
                if (callable != null) {
                    try {
                        callable.call();
                    } catch (final Exception e) {
                        android.util.Log.e(TAG, String.format("Running UI action received error.%s.", Exceptions.getStackTraceString(e)));
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

        PagerTabStrip pagerTabStrip = findViewById(R.id.pagerTabStrip);
        if (pagerTabStrip != null) {
            pagerTabStrip.setDrawFullUnderline(false);
            pagerTabStrip.setTabIndicatorColorResource(R.color.navigationColor);
            pagerTabStrip.setTextColor(Color.parseColor("#f39c12"));
        }

        final ViewPager viewPager = findViewById(R.id.pager);
        viewPager.setAdapter(new PagerAdapter(getSupportFragmentManager(), this));

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
            Log.e(TAG, String.format("Font registration failed.%s.", Exceptions.getStackTraceString(e)));
        }

        Log.d(TAG, "Listing supported camera ids.");
        listSupportedCameraIds();
        Config.ignoreSignal(Signal.SIGXCPU);
        Config.setLogLevel(Level.AV_LOG_DEBUG);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        handler.removeCallbacks(runnable);
    }

    /**
     * <p>Starts a new asynchronous FFmpeg operation with command provided.
     *
     * @param ExecuteCallback callback function to receive result of this execution
     * @param command         FFmpeg command
     */
    public static void executeAsync(final ExecuteCallback ExecuteCallback, final String command) {
        final AsyncFFmpegExecuteTask asyncCommandTask = new AsyncFFmpegExecuteTask(command, ExecuteCallback);
        asyncCommandTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public static void waitForUIAction() {
        handler.postDelayed(runnable, 250);
    }

    public static void addUIAction(final Callable<Object> callable) {
        actionQueue.add(callable);
    }

    protected void registerAppFont() throws IOException {
        final File cacheDirectory = getCacheDir();

        // SAVE FONTS
        ResourcesUtil.rawResourceToFile(getResources(), R.raw.doppioone_regular, new File(cacheDirectory, "doppioone_regular.ttf"));
        ResourcesUtil.rawResourceToFile(getResources(), R.raw.truenorg, new File(cacheDirectory, "truenorg.otf"));

        final HashMap<String, String> fontNameMapping = new HashMap<>();
        fontNameMapping.put("MyFontName", "Doppio One");
        Config.setFontDirectory(this, cacheDirectory.getAbsolutePath(), fontNameMapping);
        Config.setEnvironmentVariable("FFREPORT", String.format("file=%s", new File(cacheDirectory.getAbsolutePath(), "ffreport.txt").getAbsolutePath()));
    }

    protected void listSupportedCameraIds() {
        final List<String> supportedCameraIds = Config.getSupportedCameraIds(this);
        if (supportedCameraIds.size() == 0) {
            android.util.Log.d(MainActivity.TAG, "No supported cameras found.");
        } else {
            for (String supportedCameraId : supportedCameraIds) {
                android.util.Log.d(MainActivity.TAG, "Supported camera detected: " + supportedCameraId);
            }
        }
    }

    private Switch saf_button;
    public boolean isSafUsed() {
        return saf_button != null ? saf_button.isChecked() : false;
    }

    public void enableSaf(boolean enabled) {
        if (saf_button != null)
            saf_button.setEnabled(enabled);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            MenuInflater inflater = getMenuInflater();
            inflater.inflate(R.menu.saf_menu, menu);
            MenuItem item = (MenuItem) menu.findItem(R.id.saf_button);
            saf_button = item.getActionView().findViewById(R.id.saf_button);
            saf_button.setText("SAF");
        }
        return true;
    }

}
