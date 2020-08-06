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

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.AndroidRuntimeException;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.FFprobe;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;

import java.util.concurrent.Callable;

public class CommandTabFragment extends Fragment {

    private EditText commandText;
    private TextView outputText;
    private static final int REQUEST_SAF_FFPROBE = 11;
    private boolean backFromIntent = false;

    public CommandTabFragment() {
        super(R.layout.fragment_command_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        commandText = view.findViewById(R.id.commandText);

        View runFFmpegButton = view.findViewById(R.id.runFFmpegButton);
        runFFmpegButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                runFFmpeg(commandText.getText().toString());
            }
        });

        View runFFprobeButton = view.findViewById(R.id.runFFprobeButton);
        runFFprobeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                if (((MainActivity)requireActivity()).isSafUsed())
                    chooseInputFile();
                else
                    runFFprobe(commandText.getText().toString());
            }
        });

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());

        Log.d(MainActivity.TAG, "Last command output was: " + Config.getLastCommandOutput());
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!backFromIntent)
            setActive();
        backFromIntent = false;
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

    private void runFFmpeg(final String ffmpegCommand) {
        clearLog();

        android.util.Log.d(MainActivity.TAG, String.format("Current log level is %s.", Config.getLogLevel()));

        android.util.Log.d(MainActivity.TAG, "Testing FFmpeg COMMAND synchronously.");

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process started with arguments\n\'%s\'", ffmpegCommand));

        int result = FFmpeg.execute(ffmpegCommand);

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process exited with rc %d.", result));

        if (result != 0) {
            Popup.show(requireContext(), "Command failed. Please check output for the details.");
        }
    }

    private void runFFprobe(String ffprobeCommand) {
        clearLog();

        android.util.Log.d(MainActivity.TAG, "Testing FFprobe COMMAND synchronously.");

        android.util.Log.d(MainActivity.TAG, String.format("FFprobe process started with arguments\n\'%s\'", ffprobeCommand));

        int result = FFprobe.execute(ffprobeCommand);

        android.util.Log.d(MainActivity.TAG, String.format("FFprobe process exited with rc %d.", result));

        if (result != 0) {
            Popup.show(requireContext(), "Command failed. Please check output for the details.");
        }
    }

    private void runFFprobe(Uri inputUri) {
        if (commandText.getText().toString().trim().isEmpty()) {
            runFFprobe("-hide_banner -print_format json -show_format -show_streams " + Config.getSafParameterForRead(requireContext(), inputUri));
        }
        else {
            runFFprobe(commandText.getText().toString() + " " + Config.getSafParameterForRead(requireContext(), inputUri));
        }
    }

    private void setActive() {
        Log.i(MainActivity.TAG, "Command Tab Activated");
        enableLogCallback();
        ((MainActivity)requireActivity()).enableSaf(true);
        Popup.show(requireContext(), getString(R.string.command_test_tooltip_text));
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

    private void chooseInputFile() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT)
                .setType("*/*")
                .putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"image/*", "video/*", "audio/*"})
                .addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(intent, REQUEST_SAF_FFPROBE);
        Popup.show(requireContext(), "choose input for FFPROBE");
        backFromIntent = true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_SAF_FFPROBE && resultCode == MainActivity.RESULT_OK && data != null) {
            runFFprobe(data.getData());
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }
}
