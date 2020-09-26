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
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.DialogUtil;
import com.arthenica.mobileffmpeg.ExecuteCallback;

import java.io.File;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class AudioTabFragment extends Fragment implements AdapterView.OnItemSelectedListener {

    private AlertDialog progressDialog;
    private Button encodeButton;
    private TextView outputText;
    private String selectedCodec;

    public AudioTabFragment() {
        super(R.layout.fragment_audio_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        Spinner audioCodecSpinner = view.findViewById(R.id.audioCodecSpinner);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(requireContext(),
                R.array.audio_codec, R.layout.spinner_item);
        adapter.setDropDownViewResource(R.layout.spinner_dropdown_item);
        audioCodecSpinner.setAdapter(adapter);
        audioCodecSpinner.setOnItemSelectedListener(this);

        encodeButton = view.findViewById(R.id.encodeButton);
        encodeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                encodeAudio();
                // encodeChromaprint();
            }
        });
        encodeButton.setEnabled(false);

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());

        progressDialog = DialogUtil.createProgressDialog(requireContext(), "Encoding audio");

        selectedCodec = getResources().getStringArray(R.array.audio_codec)[0];
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static AudioTabFragment newInstance() {
        return new AudioTabFragment();
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(final LogMessage message) {
                MainActivity.addUIAction(new Callable<Object>() {

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

        android.util.Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

        MainActivity.executeAsync(new ExecuteCallback() {

            @Override
            public void apply(final long executionId, final int returnCode) {
                android.util.Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                hideProgressDialog();

                MainActivity.addUIAction(new Callable<Object>() {

                    @Override
                    public Object call() {
                        if (returnCode == RETURN_CODE_SUCCESS) {
                            Popup.show(requireContext(), "Encode completed successfully.");
                            android.util.Log.d(TAG, "Encode completed successfully.");
                        } else {
                            Popup.show(requireContext(), "Encode failed. Please check log for the details.");
                            android.util.Log.d(TAG, String.format("Encode failed with rc=%d.", returnCode));
                        }

                        return null;
                    }
                });
            }
        }, ffmpegCommand);
    }

    protected void encodeChromaprint() {
        File audioSampleFile = getAudioSampleFile();

        Log.d(TAG, "Testing AUDIO encoding with 'chromaprint' muxer");

        final String ffmpegCommand = String.format("-v quiet -i %s -f chromaprint -fp_format 2 -", audioSampleFile.getAbsolutePath());

        Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

        int returnCode = FFmpeg.execute(ffmpegCommand);

        Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));
    }

    public void createAudioSample() {
        android.util.Log.d(TAG, "Creating AUDIO sample before the test.");

        File audioSampleFile = getAudioSampleFile();
        if (audioSampleFile.exists()) {
            audioSampleFile.delete();
        }

        String ffmpegCommand = String.format("-hide_banner -y -f lavfi -i sine=frequency=1000:duration=5 -c:a pcm_s16le %s", audioSampleFile.getAbsolutePath());

        android.util.Log.d(TAG, String.format("Creating audio sample with '%s'.", ffmpegCommand));

        int result = FFmpeg.execute(ffmpegCommand);
        if (result == 0) {
            encodeButton.setEnabled(true);
            android.util.Log.d(TAG, "AUDIO sample created");
        } else {
            android.util.Log.d(TAG, String.format("Creating AUDIO sample failed with rc=%d.", result));
            Popup.show(requireContext(), "Creating AUDIO sample failed. Please check log for the details.");
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
            case "amr-nb":
                extension = "amr";
                break;
            case "amr-wb":
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
        return new File(requireContext().getFilesDir(), audio);
    }

    public File getAudioSampleFile() {
        return new File(requireContext().getFilesDir(), "audio-sample.wav");
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "Audio Tab Activated");
        disableLogCallback();
        createAudioSample();
        enableLogCallback();
        Popup.show(requireContext(), getString(R.string.audio_test_tooltip_text));
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
            case "amr-nb":
                return String.format("-hide_banner -y -i %s -ar 8000 -ab 12.2k -c:a libopencore_amrnb %s", audioSampleFile, audioOutputFile);
            case "amr-wb":
                return String.format("-hide_banner -y -i %s -ar 8000 -ab 12.2k -c:a libvo_amrwbenc -strict experimental %s", audioSampleFile, audioOutputFile);
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
