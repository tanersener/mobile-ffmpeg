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

import android.os.Bundle;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;

import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.RunCallback;
import com.arthenica.mobileffmpeg.util.AsynchronousTaskService;

import java.util.concurrent.Callable;
import java.util.concurrent.Future;

public class MainActivity extends AppCompatActivity {

    public static final String TAG = "mobile-ffmpeg-test";

    protected static AsynchronousTaskService asynchronousTaskService = new AsynchronousTaskService();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        final ViewPager viewPager = findViewById(R.id.pager);

        final PagerAdapter adapter = new PagerAdapter(getSupportFragmentManager(), this, 2);
        viewPager.setAdapter(adapter);
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

}
