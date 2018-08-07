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

import android.app.ActionBar;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.view.PagerTabStrip;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.view.MotionEvent;
import android.view.View;

import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.RunCallback;
import com.arthenica.mobileffmpeg.util.AsynchronousTaskService;

import java.util.Queue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Future;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "mobile-ffmpeg-test";

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
            supportActionBar.setCustomView(R.layout.support_action_bar);
        }

        PagerTabStrip pagerTabStrip = findViewById(R.id.pagerTabStrip);
        if (pagerTabStrip != null) {
            pagerTabStrip.setDrawFullUnderline(false);
            pagerTabStrip.setTabIndicatorColorResource(R.color.navigationColor);
            pagerTabStrip.setTextColor(Color.parseColor("#f39c12"));
        }

        final ViewPager viewPager = findViewById(R.id.pager);

        final PagerAdapter adapter = new PagerAdapter(getSupportFragmentManager(), this, 6);
        viewPager.setAdapter(adapter);

        waitForUIAction();
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
    public static Future executeAsync(final RunCallback runCallback, final String... arguments) {
        return asynchronousTaskService.runAsynchronously(new Callable<Integer>() {

            @Override
            public Integer call() {
                int returnCode = FFmpeg.execute(arguments);
                if (runCallback != null) {
                    runCallback.apply(returnCode);
                }

                return returnCode;
            }
        });
    }

    public static void registerTooltipOnTouch(final Context context, final View view, final String tooltip) {
        view.setOnTouchListener(new View.OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                Popup.show(context, tooltip);
                return true;
            }
        });
    }

    public static void waitForUIAction() {
        handler.postDelayed(runnable, 250);
    }

    public static void addUIAction(final Callable callable) {
        actionQueue.add(callable);
    }

}
