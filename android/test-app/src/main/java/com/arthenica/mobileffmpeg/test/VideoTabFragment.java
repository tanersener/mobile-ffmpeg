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
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.util.AndroidRuntimeException;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.MediaController;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.VideoView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.Statistics;
import com.arthenica.mobileffmpeg.StatisticsCallback;
import com.arthenica.mobileffmpeg.util.DialogUtil;
import com.arthenica.mobileffmpeg.util.ResourcesUtil;
import com.arthenica.smartexception.java.Exceptions;

import java.io.File;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class VideoTabFragment extends Fragment implements AdapterView.OnItemSelectedListener {

    private VideoView videoView;
    private AlertDialog progressDialog;
    private String selectedCodec;
    private Statistics statistics;

    public VideoTabFragment() {
        super(R.layout.fragment_video_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        Spinner videoCodecSpinner = view.findViewById(R.id.videoCodecSpinner);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(requireContext(),
                R.array.video_codec, R.layout.spinner_item);
        adapter.setDropDownViewResource(R.layout.spinner_dropdown_item);
        videoCodecSpinner.setAdapter(adapter);
        videoCodecSpinner.setOnItemSelectedListener(this);

        View encodeButton = view.findViewById(R.id.encodeButton);
        if (encodeButton != null) {
            encodeButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    encodeVideo();
                    // encodeWebp();
                }
            });
        }

        videoView = view.findViewById(R.id.videoPlayerFrame);

        progressDialog = DialogUtil.createProgressDialog(requireContext(), "Encoding video");

        selectedCodec = getResources().getStringArray(R.array.video_codec)[0];
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static VideoTabFragment newInstance() {
        return new VideoTabFragment();
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(LogMessage message) {
                android.util.Log.d(MainActivity.TAG, message.getText());
            }
        });
    }

    public void enableStatisticsCallback() {
        Config.enableStatisticsCallback(new StatisticsCallback() {

            @Override
            public void apply(final Statistics newStatistics) {
                MainActivity.addUIAction(new Callable<Object>() {

                    @Override
                    public Object call() {
                        VideoTabFragment.this.statistics = newStatistics;
                        updateProgressDialog();
                        return null;
                    }
                });

                throw new AndroidRuntimeException("I am test exception thrown by test application");
            }
        });
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        selectedCodec = parent.getItemAtPosition(position).toString();
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
        // DO NOTHING
    }

    public void encodeVideo() {
        final File image1File = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File image2File = new File(requireContext().getCacheDir(), "pyramid.jpg");
        final File image3File = new File(requireContext().getCacheDir(), "tajmahal.jpg");
        final File videoFile = getVideoFile();

        try {

            // IF VIDEO IS PLAYING STOP PLAYBACK
            videoView.stopPlayback();

            if (videoFile.exists()) {
                videoFile.delete();
            }

            final String videoCodec = selectedCodec;

            Log.d(TAG, String.format("Testing VIDEO encoding with '%s' codec", videoCodec));

            showProgressDialog();

            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, image1File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.pyramid, image2File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateEncodeVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath(), getSelectedVideoCodec(), getCustomOptions());

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

            long executionId = FFmpeg.executeAsync(ffmpegCommand, new ExecuteCallback() {

                @Override
                public void apply(final long executionId, final int returnCode) {
                    Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                    Log.d(TAG, "FFmpeg process output:");

                    Config.printLastCommandOutput(Log.INFO);

                    hideProgressDialog();

                    MainActivity.addUIAction(new Callable<Object>() {

                        @Override
                        public Object call() {
                            if (returnCode == RETURN_CODE_SUCCESS) {
                                Log.d(TAG, "Encode completed successfully; playing video.");
                                playVideo();
                            } else {
                                Popup.show(requireContext(), "Encode failed. Please check log for the details.");
                                Log.d(TAG, String.format("Encode failed with rc=%d.", returnCode));
                            }

                            return null;
                        }
                    });
                }
            });

            Log.d(TAG, String.format("Async FFmpeg process started with executionId %d.", executionId));

        } catch (IOException e) {
            Log.e(TAG, String.format("Encode video failed %s.", Exceptions.getStackTraceString(e)));
            Popup.show(requireContext(), "Encode video failed");
        }
    }

    protected void encodeWebp() {
        final File imageFile = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File outputFile = new File(requireContext().getFilesDir(), "video.webp");

        try {
            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, imageFile);

            Log.d(TAG, "Testing VIDEO encoding with 'webp' codec");

            final String ffmpegCommand = String.format("-hide_banner -i %s %s", imageFile.getAbsolutePath(), outputFile.getAbsolutePath());

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

            int returnCode = FFmpeg.execute(ffmpegCommand);

            Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));
        } catch (IOException e) {
            Log.e(TAG, String.format("Encode webp failed %s.", Exceptions.getStackTraceString(e)));
            Popup.show(requireContext(), "Encode webp failed");
        }
    }

    protected void playVideo() {
        MediaController mediaController = new MediaController(requireContext());
        mediaController.setAnchorView(videoView);
        videoView.setVideoURI(Uri.parse("file://" + getVideoFile().getAbsolutePath()));
        videoView.setMediaController(mediaController);
        videoView.requestFocus();
        videoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {

            @Override
            public void onPrepared(MediaPlayer mp) {
                videoView.setBackgroundColor(0x00000000);
            }
        });
        videoView.setOnErrorListener(new MediaPlayer.OnErrorListener() {

            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                videoView.stopPlayback();
                return false;
            }
        });
        videoView.start();
    }

    public String getSelectedVideoCodec() {

        // NOTE THAT MPEG4 CODEC IS ASSIGNED HERE
        String videoCodec = selectedCodec;

        // VIDEO CODEC SPINNER HAS BASIC NAMES, FFMPEG NEEDS LONGER AND EXACT CODEC NAMES.
        // APPLYING NECESSARY TRANSFORMATION HERE
        switch (videoCodec) {
            case "x264":
                videoCodec = "libx264";
                break;
            case "openh264":
                videoCodec = "libopenh264";
                break;
            case "x265":
                videoCodec = "libx265";
                break;
            case "xvid":
                videoCodec = "libxvid";
                break;
            case "vp8":
                videoCodec = "libvpx";
                break;
            case "vp9":
                videoCodec = "libvpx-vp9";
                break;
            case "aom":
                videoCodec = "libaom-av1";
                break;
            case "kvazaar":
                videoCodec = "libkvazaar";
                break;
            case "theora":
                videoCodec = "libtheora";
                break;
        }

        return videoCodec;
    }

    public File getVideoFile() {
        String videoCodec = selectedCodec;

        final String extension;
        switch (videoCodec) {
            case "vp8":
            case "vp9":
                extension = "webm";
                break;
            case "aom":
                extension = "mkv";
                break;
            case "theora":
                extension = "ogv";
                break;
            case "hap":
                extension = "mov";
                break;
            default:

                // mpeg4, x264, x265, xvid, kvazaar
                extension = "mp4";
                break;
        }

        final String video = "video." + extension;
        return new File(requireContext().getFilesDir(), video);
    }

    public String getCustomOptions() {
        String videoCodec = selectedCodec;

        switch (videoCodec) {
            case "x265":
                return "-crf 28 -preset fast ";
            case "vp8":
                return "-b:v 1M -crf 10 ";
            case "vp9":
                return "-b:v 2M ";
            case "aom":
                return "-crf 30 -strict experimental ";
            case "theora":
                return "-qscale:v 7 ";
            case "hap":
                return "-format hap_q ";
            default:

                // kvazaar, mpeg4, x264, xvid
                return "";
        }
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "Video Tab Activated");
        enableLogCallback();
        enableStatisticsCallback();
        Popup.show(requireContext(), Tooltip.VIDEO_TEST_TOOLTIP_TEXT);
    }

    protected void showProgressDialog() {

        // CLEAN STATISTICS
        statistics = null;
        Config.resetStatistics();

        progressDialog.show();
    }

    protected void updateProgressDialog() {
        if (statistics == null) {
            return;
        }

        int timeInMilliseconds = this.statistics.getTime();
        if (timeInMilliseconds > 0) {
            int totalVideoDuration = 9000;

            String completePercentage = new BigDecimal(timeInMilliseconds).multiply(new BigDecimal(100)).divide(new BigDecimal(totalVideoDuration), 0, BigDecimal.ROUND_HALF_UP).toString();

            TextView textView = progressDialog.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(String.format("Encoding video: %% %s.", completePercentage));
            }
        }
    }

    protected void hideProgressDialog() {
        progressDialog.dismiss();

        MainActivity.addUIAction(new Callable<Object>() {

            @Override
            public Object call() {
                VideoTabFragment.this.progressDialog = DialogUtil.createProgressDialog(requireContext(), "Encoding video");
                return null;
            }
        });
    }

}
