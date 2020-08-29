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
import android.content.Intent;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.AndroidRuntimeException;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.MediaController;
import android.widget.TextView;
import android.widget.VideoView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.FFprobe;
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

import static android.app.Activity.RESULT_OK;
import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;
import static com.arthenica.mobileffmpeg.test.MainActivity.handler;

public class SafTabFragment extends Fragment {

    private TextView outputText;
    private Uri inUri;
    private Uri outUri;
    private static final int REQUEST_SAF_FFPROBE = 11;
    private static final int REQUEST_SAF_FFMPEG = 12;

    private boolean backFromIntent = false;

    private VideoView videoView;
    private AlertDialog progressDialog;
    private Statistics statistics;

    private Button runFFmpegButton;
    private Button runFFprobeButton;

    public SafTabFragment() {
        super(R.layout.fragment_saf_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        runFFmpegButton = view.findViewById(R.id.runFFmpegButton);
        runFFmpegButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT)
                        .setType("video/*")
                        .putExtra(Intent.EXTRA_TITLE, "video.mp4")
                        .addCategory(Intent.CATEGORY_OPENABLE);
                startActivityForResult(intent, REQUEST_SAF_FFMPEG);
            }
        });

        runFFprobeButton = view.findViewById(R.id.runFFprobeButton);
        runFFprobeButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT)
                        .setType("*/*")
                        .putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"image/*", "video/*", "audio/*"})
                        .addCategory(Intent.CATEGORY_OPENABLE);
                startActivityForResult(intent, REQUEST_SAF_FFPROBE);
            }
        });

        outputText = view.findViewById(R.id.outputText);
        outputText.setMovementMethod(new ScrollingMovementMethod());

        videoView = view.findViewById(R.id.videoPlayerFrame);
        progressDialog = DialogUtil.createProgressDialog(requireContext(), "Encoding video");
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    static SafTabFragment newInstance() {
        return new SafTabFragment();
    }

    private void enableLogCallback() {
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

    private void runFFprobe() {
        videoView.setVisibility(View.GONE);
        outputText.setVisibility(View.VISIBLE);

        clearLog();

        final String ffprobeCommand = "-hide_banner -print_format json -show_format -show_streams " + Config.getSafParameterForRead(getContext(), inUri);

        Log.d(MainActivity.TAG, "Testing FFprobe COMMAND synchronously.");

        Log.d(MainActivity.TAG, String.format("FFprobe process started with arguments\n\'%s\'", ffprobeCommand));

        int result = FFprobe.execute(ffprobeCommand);

        Log.d(MainActivity.TAG, String.format("FFprobe process exited with rc %d", result));

        if (result != 0) {
            Popup.show(requireContext(), "Command failed. Please check output for the details.");
        }
        inUri = null;
    }

    private void setActive() {
        if (backFromIntent) {
            backFromIntent = false;
            return;
        }
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            Popup.show(requireContext(), "SAF is only available for Android 4.4 and above.");
            runFFprobeButton.setEnabled(false);
            runFFmpegButton.setEnabled(false);
            outputText.setEnabled(false);
            Log.i(MainActivity.TAG, "SAF Tab Dectivated");
            return;
        }
        Log.i(MainActivity.TAG, "SAF Tab Activated");
        enableLogCallback();
        enableStatisticsCallback();
        Popup.show(requireContext(), getString(R.string.saf_test_tooltip_text));
    }

    private void appendLog(final String logMessage) {
        outputText.append(logMessage);
    }

    private void clearLog() {
        outputText.setText("");
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        backFromIntent = true;
        if (requestCode == REQUEST_SAF_FFPROBE && resultCode == RESULT_OK && data != null) {
            inUri = data.getData();
            handler.post(new Runnable() {
                @Override
                public void run() {
                    runFFprobe();
                }
            });
        } else if (requestCode == REQUEST_SAF_FFMPEG && resultCode == MainActivity.RESULT_OK && data != null) {
            outUri = data.getData();
            handler.post(new Runnable() {
                @Override
                public void run() {
                    encodeVideo();
                }
            });
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }

    private String getCodec(String videoPath) {
        String extension = "mp4";
        int pos = videoPath.lastIndexOf('.');
        if (pos >= 0)
            extension = videoPath.substring(pos+1);

        switch (extension) {
            case "webm":
                return "vp8";
            case "mkv":
                return "aom";
            case "ogv":
                return "theora";
            case "mov":
                return "hap";
            case "mp4":
            default:
                return "mpeg4";
        }
    }

    private String getCustomOptions(String videoCodec) {
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

    private void enableStatisticsCallback() {
        Config.enableStatisticsCallback(new StatisticsCallback() {

            @Override
            public void apply(final Statistics newStatistics) {
                MainActivity.addUIAction(new Callable<Object>() {

                    @Override
                    public Object call() {
                        statistics = newStatistics;
                        updateProgressDialog();
                        return null;
                    }
                });

                throw new AndroidRuntimeException("I am test exception thrown by test application");
            }
        });
    }

    private void encodeVideo() {
        final File image1File = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File image2File = new File(requireContext().getCacheDir(), "pyramid.jpg");
        final File image3File = new File(requireContext().getCacheDir(), "tajmahal.jpg");
        final String videoPath = Config.getSafParameterForReadAndWrite(requireContext(), outUri);

        try {
            // IF VIDEO IS PLAYING STOP PLAYBACK
            videoView.stopPlayback();

            videoView.setVisibility(View.GONE);
            outputText.setVisibility(View.VISIBLE);

            String selectedCodec = getCodec(videoPath);
            Log.d(TAG, String.format("Testing VIDEO encoding with '%s' codec", selectedCodec));

            showProgressDialog();

            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, image1File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.pyramid, image2File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateEncodeVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoPath, selectedCodec, getCustomOptions(selectedCodec));

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

    private void playVideo() {
        videoView.setVisibility(View.VISIBLE);
        outputText.setVisibility(View.GONE);

        MediaController mediaController = new MediaController(requireContext());
        mediaController.setAnchorView(videoView);
        videoView.setVideoURI(outUri);
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
        videoView.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
              @Override
              public void onCompletion(MediaPlayer mediaPlayer) {
                  inUri = outUri;
                  handler.post(new Runnable() {
                      @Override
                      public void run() {
                          runFFprobe();
                      }
                  });
              }
        });
        videoView.start();
    }

    private void showProgressDialog() {

        // CLEAN STATISTICS
        statistics = null;
        Config.resetStatistics();

        progressDialog.show();
    }

    private void updateProgressDialog() {
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

    private void hideProgressDialog() {
        progressDialog.dismiss();

        MainActivity.addUIAction(new Callable<Object>() {

            @Override
            public Object call() {
                progressDialog = DialogUtil.createProgressDialog(requireContext(), "Encoding video");
                return null;
            }
        });
    }

}
