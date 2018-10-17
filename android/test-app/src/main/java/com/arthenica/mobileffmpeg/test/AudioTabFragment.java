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

import android.app.AlertDialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.text.method.ScrollingMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.RunCallback;

import java.io.File;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.FFmpeg.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class AudioTabFragment extends Fragment implements AdapterView.OnItemSelectedListener {

    private MainActivity mainActivity;
    private AlertDialog progressDialog;
    private Button encodeButton;
    private TextView outputText;
    private String selectedCodec;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_audio_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            Spinner audioCodecSpinner = getView().findViewById(R.id.audioCodecSpinner);
            ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(mainActivity,
                    R.array.audio_codec, R.layout.spinner_item);
            adapter.setDropDownViewResource(R.layout.spinner_dropdown_item);
            audioCodecSpinner.setAdapter(adapter);
            audioCodecSpinner.setOnItemSelectedListener(this);

            encodeButton = getView().findViewById(R.id.encodeButton);
            encodeButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    encodeAudio();
                }
            });
            encodeButton.setEnabled(false);

            outputText = getView().findViewById(R.id.outputText);
            outputText.setMovementMethod(new ScrollingMovementMethod());
        }

        progressDialog = mainActivity.createProgressDialog("Encoding audio");

        selectedCodec = getResources().getStringArray(R.array.audio_codec)[0];
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

    public static AudioTabFragment newInstance(final MainActivity mainActivity) {
        final AudioTabFragment fragment = new AudioTabFragment();
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

    public void disableLogCallback() {
        Config.enableLogCallback(null);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        selectedCodec = parent.getItemAtPosition(position).toString();
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
        // DO NOTHING
    }

    public void encodeAudio() {
        File audioOutputFile = getAudioOutputFile();
        if (audioOutputFile.exists()) {
            audioOutputFile.delete();
        }

        final String audioCodec = selectedCodec;

        android.util.Log.d(TAG, String.format("Testing AUDIO encoding with '%s' codec", audioCodec));

        final String ffmpegCommand = generateAudioEncodeScript();

        showProgressDialog();

        clearLog();

        android.util.Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", ffmpegCommand));

        MainActivity.executeAsync(new RunCallback() {

            @Override
            public void apply(final int returnCode) {
                android.util.Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                hideProgressDialog();

                MainActivity.addUIAction(new Callable() {

                    @Override
                    public Object call() {
                        if (returnCode == RETURN_CODE_SUCCESS) {
                            Popup.show(mainActivity, "Encode completed successfully.");
                            android.util.Log.d(TAG, "Encode completed successfully.");
                        } else {
                            Popup.show(mainActivity, "Encode failed. Please check log for the details.");
                            android.util.Log.d(TAG, String.format("Encode failed with rc=%d", returnCode));
                        }

                        return null;
                    }
                });
            }
        }, ffmpegCommand);
    }

    public void createAudioSample() {
        android.util.Log.d(TAG, "Creating AUDIO sample before the test.");

        File audioSampleFile = getAudioSampleFile();
        if (audioSampleFile.exists()) {
            audioSampleFile.delete();
        }

        String ffmpegCommand = String.format("-hide_banner -y -f lavfi -i sine=frequency=1000:duration=5 -c:a pcm_s16le %s", audioSampleFile.getAbsolutePath());

        android.util.Log.d(TAG, String.format("Sample file is created with '%s'", ffmpegCommand));

        int result = FFmpeg.execute(ffmpegCommand);
        if (result == 0) {
            encodeButton.setEnabled(true);
            android.util.Log.d(TAG, "AUDIO sample created");
        } else {
            android.util.Log.d(TAG, String.format("Creating AUDIO sample failed with rc=%d", result));
            Popup.show(mainActivity, "Creating AUDIO sample failed. Please check log for the details.");
        }
    }

    public File getAudioOutputFile() {
        String audioCodec = selectedCodec;

        String extension;
        switch (audioCodec) {
            case "mp2 (twolame)":
                extension = "mpg";
                break;
            case "mp3 (liblame)":
            case "mp3 (libshine)":
                extension = "mp3";
                break;
            case "vorbis":
                extension = "ogg";
                break;
            case "opus":
                extension = "opus";
                break;
            case "amr":
                extension = "amr";
                break;
            case "ilbc":
                extension = "lbc";
                break;
            case "speex":
                extension = "spx";
                break;
            case "wavpack":
                extension = "wv";
                break;
            default:

                // soxr
                extension = "wav";
                break;
        }

        final String audio = "audio." + extension;
        return new File(mainActivity.getFilesDir(), audio);
    }

    public File getAudioSampleFile() {
        return new File(mainActivity.getFilesDir(), "audio-sample.wav");
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "Audio Tab Activated");
        disableLogCallback();
        createAudioSample();
        enableLogCallback();
        Popup.show(mainActivity, Tooltip.AUDIO_TEST_TOOLTIP_TEXT);
    }

    public void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    public void clearLog() {
        outputText.setText("");
    }

    protected void showProgressDialog() {
        progressDialog.show();
    }

    protected void hideProgressDialog() {
        progressDialog.dismiss();
    }

    public String generateAudioEncodeScript() {
        String audioCodec = selectedCodec;
        String audioSampleFile = getAudioSampleFile().getAbsolutePath();
        String audioOutputFile = getAudioOutputFile().getAbsolutePath();

        switch (audioCodec) {
            case "mp2 (twolame)":
                return String.format("-hide_banner -y -i %s -c:a mp2 -b:a 192k %s", audioSampleFile, audioOutputFile);
            case "mp3 (liblame)":
                return String.format("-hide_banner -y -i %s -c:a libmp3lame -qscale:a 2 %s", audioSampleFile, audioOutputFile);
            case "mp3 (libshine)":
                return String.format("-hide_banner -y -i %s -c:a libshine -qscale:a 2 %s", audioSampleFile, audioOutputFile);
            case "vorbis":
                return String.format("-hide_banner -y -i %s -c:a libvorbis -b:a 64k %s", audioSampleFile, audioOutputFile);
            case "opus":
                return String.format("-hide_banner -y -i %s -c:a libopus -b:a 64k -vbr on -compression_level 10 %s", audioSampleFile, audioOutputFile);
            case "amr":
                return String.format("-hide_banner -y -i %s -ar 8000 -ab 12.2k %s", audioSampleFile, audioOutputFile);
            case "ilbc":
                return String.format("-hide_banner -y -i %s -c:a ilbc -ar 8000 -b:a 15200 %s", audioSampleFile, audioOutputFile);
            case "speex":
                return String.format("-hide_banner -y -i %s -c:a libspeex -ar 16000 %s", audioSampleFile, audioOutputFile);
            case "wavpack":
                return String.format("-hide_banner -y -i %s -c:a wavpack -b:a 64k %s", audioSampleFile, audioOutputFile);
            default:

                // soxr
                return String.format("-hide_banner -y -i %s -af aresample=resampler=soxr -ar 44100 %s", audioSampleFile, audioOutputFile);
        }
    }

}
