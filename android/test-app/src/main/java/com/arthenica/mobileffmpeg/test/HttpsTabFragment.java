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
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFprobe;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.MediaInformation;
import com.arthenica.mobileffmpeg.StreamInformation;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Iterator;
import java.util.concurrent.Callable;

public class HttpsTabFragment extends Fragment {

    public static final String HTTPS_TEST_DEFAULT_URL = "https://download.blender.org/peach/trailer/trailer_1080p.ogg";

    private EditText urlText;
    private TextView outputText;

    public HttpsTabFragment() {
        super(R.layout.fragment_https_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        urlText = view.findViewById(R.id.urlText);

        View getInfoButton = view.findViewById(R.id.getInfoButton);
        getInfoButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                getInfo();
            }
        });

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static HttpsTabFragment newInstance() {
        return new HttpsTabFragment();
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
        MediaInformation information = FFprobe.getMediaInformation(testUrl);
        if (information == null) {
            appendLog("Get media information failed\n");
        } else {
            appendLog("Media information for " + information.getFilename() + "\n");

            if (information.getFormat() != null) {
                appendLog("Format: " + information.getFormat() + "\n");
            }
            if (information.getBitrate() != null) {
                appendLog("Bitrate: " + information.getBitrate() + "\n");
            }
            if (information.getDuration() != null) {
                appendLog("Duration: " + information.getDuration() + "\n");
            }
            if (information.getStartTime() != null) {
                appendLog("Start time: " + information.getStartTime() + "\n");
            }
            if (information.getTags() != null) {
                JSONObject tags = information.getTags();
                if (tags != null) {
                    Iterator<String> keys = tags.keys();
                    while (keys.hasNext()) {
                        try {
                            String next = keys.next();
                            appendLog("Tag: " + next + ":" + tags.getString(next) + "\n");
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
            if (information.getStreams() != null) {
                for (StreamInformation stream : information.getStreams()) {
                    if (stream.getIndex() != null) {
                        appendLog("Stream index: " + stream.getIndex() + "\n");
                    }
                    if (stream.getType() != null) {
                        appendLog("Stream type: " + stream.getType() + "\n");
                    }
                    if (stream.getCodec() != null) {
                        appendLog("Stream codec: " + stream.getCodec() + "\n");
                    }
                    if (stream.getFullCodec() != null) {
                        appendLog("Stream full codec: " + stream.getFullCodec() + "\n");
                    }
                    if (stream.getFormat() != null) {
                        appendLog("Stream format: " + stream.getFormat() + "\n");
                    }

                    if (stream.getWidth() != null) {
                        appendLog("Stream width: " + stream.getWidth() + "\n");
                    }
                    if (stream.getHeight() != null) {
                        appendLog("Stream height: " + stream.getHeight() + "\n");
                    }

                    if (stream.getBitrate() != null) {
                        appendLog("Stream bitrate: " + stream.getBitrate() + "\n");
                    }
                    if (stream.getSampleRate() != null) {
                        appendLog("Stream sample rate: " + stream.getSampleRate() + "\n");
                    }
                    if (stream.getSampleFormat() != null) {
                        appendLog("Stream sample format: " + stream.getSampleFormat() + "\n");
                    }
                    if (stream.getChannelLayout() != null) {
                        appendLog("Stream channel layout: " + stream.getChannelLayout() + "\n");
                    }

                    if (stream.getSampleAspectRatio() != null) {
                        appendLog("Stream sample aspect ratio: " + stream.getSampleAspectRatio() + "\n");
                    }
                    if (stream.getDisplayAspectRatio() != null) {
                        appendLog("Stream display ascpect ratio: " + stream.getDisplayAspectRatio() + "\n");
                        ;
                    }
                    if (stream.getAverageFrameRate() != null) {
                        appendLog("Stream average frame rate: " + stream.getAverageFrameRate() + "\n");
                    }
                    if (stream.getRealFrameRate() != null) {
                        appendLog("Stream real frame rate: " + stream.getRealFrameRate() + "\n");
                    }
                    if (stream.getTimeBase() != null) {
                        appendLog("Stream time base: " + stream.getTimeBase() + "\n");
                    }
                    if (stream.getCodecTimeBase() != null) {
                        appendLog("Stream codec time base: " + stream.getCodecTimeBase() + "\n");
                    }

                    if (stream.getTags() != null) {
                        JSONObject tags = stream.getTags();
                        if (tags != null) {
                            Iterator<String> keys = tags.keys();
                            while (keys.hasNext()) {
                                try {
                                    String next = keys.next();
                                    appendLog("Stream tag: " + next + ":" + tags.getString(next) + "\n");
                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "Https Tab Activated");
        enableLogCallback();
        Popup.show(requireContext(), Tooltip.HTTPS_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

}
