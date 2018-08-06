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

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.Log;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.RunCallback;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class CommandTabFragment extends Fragment {

    private Context context;
    private EditText commandText;
    private TextView logText;
    private final Queue<String> logQueue;

    public CommandTabFragment() {
        logQueue = new ConcurrentLinkedQueue<>();
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);

        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            android.util.Log.i(MainActivity.TAG, "COMMAND TAB VIEWED");
        }
    }

    public void enableLogCallback() {
        Log.enableLogCallback(new LogCallback() {

            @Override
            public void apply(Log.Message message) {
                logQueue.add(message.getText());
            }
        });
    }

    public static CommandTabFragment newInstance(final Context context) {
        CommandTabFragment fragment = new CommandTabFragment();
        fragment.setContext(context);
        return fragment;
    }

    public void setContext(Context context) {
        this.context = context;
    }

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_command_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            commandText = getView().findViewById(R.id.commandText);

            // CHANGE LOG TEXT COLOR
            logText = getView().findViewById(R.id.logText);
            logText.setBackgroundColor(Color.LTGRAY);
            logText.setMovementMethod(new ScrollingMovementMethod());

            View runButton = getView().findViewById(R.id.runButton);
            runButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    runFFmpeg();
                }
            });

            View runAsyncButton = getView().findViewById(R.id.runAsyncButton);
            runAsyncButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    runFFmpegAsync();
                }
            });

            waitForLogs();
        }
    }

    public void runFFmpeg() {
        enableLogCallback();

        String command = commandText.getText().toString();
        String[] split = command.split(" ");

        clearLog();

        int returnCode = FFmpeg.execute(split);
        android.util.Log.i(MainActivity.TAG, String.format("Process exited with rc %d.", returnCode));
        Toast.makeText(context, "Run completed", Toast.LENGTH_SHORT).show();
    }

    public void runFFmpegAsync() {
        String command = commandText.getText().toString();
        String[] arguments = command.split(" ");

        clearLog();

        MainActivity.executeAsync(new RunCallback() {

            @Override
            public void apply(int returnCode) {
                android.util.Log.i(MainActivity.TAG, String.format("Async process exited with rc %d.", returnCode));
            }
        }, arguments);
    }

    public void appendLog(final String logMessage) {
        logText.append(logMessage);
    }

    public void clearLog() {
        logQueue.clear();
        logText.setText("");
    }

    public void waitForLogs() {
        final Handler handler = new Handler();
        final Runnable runnable = new Runnable() {

            @Override
            public void run() {
                String logMessage;

                do {
                    logMessage = logQueue.poll();
                    if (logMessage != null) {
                        appendLog(logMessage);
                    }
                } while (logMessage != null);

                handler.postDelayed(this, 250);
            }
        };

        handler.postDelayed(runnable, 250);
    }

}
