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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.MediaInformation;
import com.arthenica.mobileffmpeg.StreamInformation;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;

public class HttpsTabFragment extends Fragment {

    public static final String HTTPS_TEST_DEFAULT_URL = "https://download.blender.org/peach/trailer/trailer_1080p.ogg";

    private MainActivity mainActivity;
    private EditText urlText;
    private TextView outputText;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_https_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            urlText = getView().findViewById(R.id.urlText);

            View getInfoButton = getView().findViewById(R.id.getInfoButton);
            getInfoButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    getInfo();
                }
            });

            outputText = getView().findViewById(R.id.outputText);
            outputText.setMovementMethod(new ScrollingMovementMethod());
        }
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

    public static HttpsTabFragment newInstance(final MainActivity mainActivity) {
        final HttpsTabFragment fragment = new HttpsTabFragment();
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
            }
        });
    }

    public void getInfo() {
        clearLog();

        String testUrl = urlText.getText().toString();
        if (testUrl.isEmpty()) {
            testUrl = HTTPS_TEST_DEFAULT_URL;
            urlText.setText(testUrl);
            android.util.Log.d(MainActivity.TAG, String.format("Testing HTTPS with default url '%s'", testUrl));
        } else {
            android.util.Log.d(MainActivity.TAG, String.format("Testing HTTPS with url '%s'", testUrl));
        }

        // HTTPS COMMAND ARGUMENTS
        MediaInformation information = FFmpeg.getMediaInformation(testUrl);
        if (information == null) {
            android.util.Log.d(MainActivity.TAG, "Get media information failed");
        } else {
            android.util.Log.d(MainActivity.TAG, "Media information for " + information.getPath());

            if (information.getFormat() != null) {
                android.util.Log.d(MainActivity.TAG, "Format: " + information.getFormat());
            }
            if (information.getBitrate() != null) {
                android.util.Log.d(MainActivity.TAG, "Bitrate: " + information.getBitrate());
            }
            if (information.getDuration() != null) {
                android.util.Log.d(MainActivity.TAG, "Duration: " + information.getDuration());
            }
            if (information.getStartTime() != null) {
                android.util.Log.d(MainActivity.TAG, "Start time: " + information.getStartTime());
            }
            if (information.getMetadataEntries() != null) {
                Set<Map.Entry<String, String>> entries = information.getMetadataEntries();
                for (Map.Entry<String, String> entry : entries) {
                    android.util.Log.d(MainActivity.TAG, "Metadata: " + entry.getKey() + ":" + entry.getValue());
                }
            }
            if (information.getStreams() != null) {
                for (StreamInformation stream : information.getStreams()) {
                    if (stream.getIndex() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream index: " + stream.getIndex());
                    }
                    if (stream.getType() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream type: " + stream.getType());
                    }
                    if (stream.getCodec() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream codec: " + stream.getCodec());
                    }
                    if (stream.getFullCodec() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream full codec: " + stream.getFullCodec());
                    }
                    if (stream.getFormat() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream format: " + stream.getFormat());
                    }
                    if (stream.getFullFormat() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream full format: " + stream.getFullFormat());
                    }

                    if (stream.getWidth() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream width: " + stream.getWidth());
                    }
                    if (stream.getHeight() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream height: " + stream.getHeight());
                    }

                    if (stream.getBitrate() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream bitrate: " + stream.getBitrate());
                    }
                    if (stream.getSampleRate() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream sample rate: " + stream.getSampleRate());
                    }
                    if (stream.getSampleFormat() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream sample format: " + stream.getSampleFormat());
                    }
                    if (stream.getChannelLayout() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream channel layout: " + stream.getChannelLayout());
                    }

                    if (stream.getSampleAspectRatio() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream sample aspect ratio: " + stream.getSampleAspectRatio());
                    }
                    if (stream.getDisplayAspectRatio() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream display ascpect ratio: " + stream.getDisplayAspectRatio())
                        ;
                    }
                    if (stream.getAverageFrameRate() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream average frame rate: " + stream.getAverageFrameRate());
                    }
                    if (stream.getRealFrameRate() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream real frame rate: " + stream.getRealFrameRate());
                    }
                    if (stream.getTimeBase() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream time base: " + stream.getTimeBase());
                    }
                    if (stream.getCodecTimeBase() != null) {
                        android.util.Log.d(MainActivity.TAG, "Stream codec time base: " + stream.getCodecTimeBase());
                    }

                    if (stream.getMetadataEntries() != null) {
                        Set<Map.Entry<String, String>> entries = stream.getMetadataEntries();
                        for (Map.Entry<String, String> entry : entries) {
                            android.util.Log.d(MainActivity.TAG, "Stream metadata: " + entry.getKey() + ":" + entry.getValue());
                        }
                    }
                }
            }
        }
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "Https Tab Activated");
        enableLogCallback();
        Popup.show(mainActivity, Tooltip.HTTPS_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

}
