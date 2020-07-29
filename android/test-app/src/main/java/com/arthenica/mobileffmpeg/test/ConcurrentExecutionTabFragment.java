/*
 * Copyright (c) 2020 Taner Sener
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
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.FFmpegExecution;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.ResourcesUtil;
import com.arthenica.smartexception.java.Exceptions;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_CANCEL;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class ConcurrentExecutionTabFragment extends Fragment {

    private TextView outputText;
    private long executionId1;
    private long executionId2;
    private long executionId3;

    public ConcurrentExecutionTabFragment() {
        super(R.layout.fragment_concurrent_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        View encodeButton1 = view.findViewById(R.id.encodeButton1);
        if (encodeButton1 != null) {
            encodeButton1.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    encodeVideo(1);
                }
            });
        }

        View encodeButton2 = view.findViewById(R.id.encodeButton2);
        if (encodeButton2 != null) {
            encodeButton2.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    encodeVideo(2);
                }
            });
        }

        View encodeButton3 = view.findViewById(R.id.encodeButton3);
        if (encodeButton3 != null) {
            encodeButton3.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    encodeVideo(3);
                }
            });
        }

        View cancelButton1 = view.findViewById(R.id.cancelButton1);
        if (cancelButton1 != null) {
            cancelButton1.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    cancel(1);
                }
            });
        }

        View cancelButton2 = view.findViewById(R.id.cancelButton2);
        if (cancelButton2 != null) {
            cancelButton2.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    cancel(2);
                }
            });
        }

        View cancelButton3 = view.findViewById(R.id.cancelButton3);
        if (cancelButton3 != null) {
            cancelButton3.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    cancel(3);
                }
            });
        }

        View cancelButtonAll = view.findViewById(R.id.cancelButtonAll);
        if (cancelButtonAll != null) {
            cancelButtonAll.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    cancel(0);
                }
            });
        }

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static ConcurrentExecutionTabFragment newInstance() {
        return new ConcurrentExecutionTabFragment();
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(final LogMessage message) {
                MainActivity.addUIAction(new Callable<Object>() {

                    @Override
                    public Object call() {
                        appendLog(String.format(Locale.getDefault(), "%d:%s", message.getExecutionId(), message.getText()));
                        return null;
                    }
                });
            }
        });
    }

    public void encodeVideo(final int buttonNumber) {
        final File image1File = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File image2File = new File(requireContext().getCacheDir(), "pyramid.jpg");
        final File image3File = new File(requireContext().getCacheDir(), "tajmahal.jpg");
        final File videoFile = new File(requireContext().getFilesDir(), String.format(Locale.getDefault(), "video%d.mp4", buttonNumber));

        try {

            Log.d(TAG, String.format("Testing CONCURRENT EXECUTION for button %d.", buttonNumber));

            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, image1File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.pyramid, image2File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateEncodeVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath(), "mpeg4", "");

            Log.d(TAG, String.format("FFmpeg process starting for button %d with arguments\n'%s'.", buttonNumber, ffmpegCommand));

            long executionId = FFmpeg.executeAsync(ffmpegCommand, new ExecuteCallback() {

                @Override
                public void apply(final long executionId, final int returnCode) {
                    if (returnCode == RETURN_CODE_CANCEL) {
                        Log.d(TAG, String.format("FFmpeg process ended with cancel for button %d with executionId %d.", buttonNumber, executionId));
                    } else {
                        Log.d(TAG, String.format("FFmpeg process ended with rc %d for button %d with executionId %d.", returnCode, buttonNumber, executionId));
                    }
                }
            });

            Log.d(TAG, String.format("Async FFmpeg process started for button %d with executionId %d.", buttonNumber, executionId));

            switch (buttonNumber) {
                case 1: {
                    executionId1 = executionId;
                }
                break;
                case 2: {
                    executionId2 = executionId;
                }
                break;
                default: {
                    executionId3 = executionId;
                }
            }

        } catch (IOException e) {
            Log.e(TAG, String.format("Encode video failed %s.", Exceptions.getStackTraceString(e)));
            Popup.show(requireContext(), "Encode video failed");
        }

        listFFmpegExecutions();
    }

    public void listFFmpegExecutions() {
        final List<FFmpegExecution> ffmpegExecutions = FFmpeg.listExecutions();
        Log.d(TAG, "Listing ongoing FFmpeg executions.");
        for (int i = 0; i < ffmpegExecutions.size(); i++) {
            FFmpegExecution execution = ffmpegExecutions.get(i);
            Log.d(TAG, String.format("Execution %d = id:%d, startTime:%s, command:%s.", i, execution.getExecutionId(), execution.getStartTime(), execution.getCommand()));
        }
        Log.d(TAG, "Listed ongoing FFmpeg executions.");
    }

    public void cancel(final int buttonNumber) {
        long executionId = 0;

        switch (buttonNumber) {
            case 1: {
                executionId = executionId1;
            }
            break;
            case 2: {
                executionId = executionId2;
            }
            break;
            case 3: {
                executionId = executionId3;
            }
        }

        Log.d(TAG, String.format("Cancelling FFmpeg process for button %d with executionId %d.", buttonNumber, executionId));

        if (executionId == 0) {
            FFmpeg.cancel();
        } else {
            FFmpeg.cancel(executionId);
        }
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "Concurrent Execution Tab Activated");
        enableLogCallback();
        Popup.show(requireContext(), Tooltip.CONCURRENT_EXECUTION_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

}
