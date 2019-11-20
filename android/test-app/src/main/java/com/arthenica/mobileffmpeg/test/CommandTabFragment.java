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
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.text.method.ScrollingMovementMethod;
import android.util.AndroidRuntimeException;
import android.util.Log;
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

    private EditText commandText;
    private TextView outputText;

    public CommandTabFragment() {
        super(R.layout.fragment_command_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        commandText = view.findViewById(R.id.commandText);

        View runButton = view.findViewById(R.id.runButton);
        runButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                runFFmpeg();
            }
        });

        View runAsyncButton = view.findViewById(R.id.runAsyncButton);
        runAsyncButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                runFFmpegAsync();
            }
        });

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());

        Log.d(MainActivity.TAG, "Last command output was: " + FFmpeg.getLastCommandOutput());
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static CommandTabFragment newInstance() {
        return new CommandTabFragment();
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
            Popup.show(requireContext(), "Command failed. Please check output for the details.");
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
                            Popup.show(requireContext(), "Command failed. Please check output for the details.");
                            return null;
                        }
                    });
                }

            }
        }, ffmpegCommand);
    }

    private void setActive() {
        Log.i(MainActivity.TAG, "Command Tab Activated");
        enableLogCallback();
        Popup.show(requireContext(), Tooltip.COMMAND_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

}
