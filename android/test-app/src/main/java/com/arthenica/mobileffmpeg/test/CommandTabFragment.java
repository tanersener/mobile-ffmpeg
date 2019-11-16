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
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.method.ScrollingMovementMethod;
import android.util.AndroidRuntimeException;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.ExecuteCallback;

import java.util.concurrent.Callable;

public class CommandTabFragment extends Fragment {

    private MainActivity mainActivity;
    private EditText commandText;
    private TextView outputText;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_command_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            commandText = getView().findViewById(R.id.commandText);

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

            outputText = getView().findViewById(R.id.outputText);
            outputText.setMovementMethod(new ScrollingMovementMethod());
        }

        android.util.Log.d(MainActivity.TAG, "Last command output was: " + FFmpeg.getLastCommandOutput());
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            setActive();
        }
    }

    public void setMainActivity(MainActivity mainActivity) {
        this.mainActivity = mainActivity;
    }

    public static CommandTabFragment newInstance(final MainActivity mainActivity) {
        final CommandTabFragment fragment = new CommandTabFragment();
        fragment.setMainActivity(mainActivity);
        return fragment;
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(final LogMessage message) {
                MainActivity.addUIAction(new Callable() {

                    @Override
                    public Object call() {
                        appendLog(message.getText());
                        return null;
                    }
                });

                throw new AndroidRuntimeException("I am test exception thrown by test application");
            }
        });
    }

    public void runFFmpeg() {
        clearLog();

        final String ffmpegCommand = String.format("-hide_banner %s", commandText.getText().toString());

        android.util.Log.d(MainActivity.TAG, "Testing COMMAND synchronously.");

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process started with arguments\n\'%s\'", ffmpegCommand));

        int result = FFmpeg.execute(ffmpegCommand);

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process exited with rc %d", result));

        if (result != 0) {
            Popup.show(mainActivity, "Command failed. Please check output for the details.");
        }
    }

    public void runFFmpegAsync() {
        clearLog();

        final String ffmpegCommand = String.format("-hide_banner %s", commandText.getText().toString());

        android.util.Log.d(MainActivity.TAG, "Testing COMMAND asynchronously.");

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process started with arguments\n\'%s\'", ffmpegCommand));

        MainActivity.executeAsync(new ExecuteCallback() {

            @Override
            public void apply(int result, final String commandOutput) {

                android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process exited with rc %d", result));

                if (result != 0) {
                    MainActivity.addUIAction(new Callable() {
                        @Override
                        public Object call() {
                            Popup.show(mainActivity, "Command failed. Please check output for the details.");
                            return null;
                        }
                    });
                }

            }
        }, ffmpegCommand);
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "Command Tab Activated");
        enableLogCallback();
        Popup.show(mainActivity, Tooltip.COMMAND_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

}
