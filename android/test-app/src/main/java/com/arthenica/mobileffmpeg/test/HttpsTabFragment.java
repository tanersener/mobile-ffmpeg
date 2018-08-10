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

import java.util.concurrent.Callable;

public class HttpsTabFragment extends Fragment {

    public static final String HTTPS_TEST_DEFAULT_URL = "https://download.blender.org/peach/trailer/trailer_400p.ogg";

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
        final String ffmpegCommand = String.format("-hide_banner -i %s", testUrl);

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process started with arguments\n'%s'", ffmpegCommand));

        // EXECUTE
        int result = FFmpeg.execute(ffmpegCommand);

        android.util.Log.d(MainActivity.TAG, String.format("FFmpeg process exited with rc %d", result));
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
