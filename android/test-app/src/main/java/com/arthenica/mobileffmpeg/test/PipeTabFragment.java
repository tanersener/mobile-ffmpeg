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
import android.os.AsyncTask;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.MediaController;
import android.widget.TextView;
import android.widget.VideoView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.Statistics;
import com.arthenica.mobileffmpeg.StatisticsCallback;
import com.arthenica.mobileffmpeg.util.AsyncCatImageTask;
import com.arthenica.mobileffmpeg.util.ExecuteCallback;

import java.io.File;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.FFmpeg.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class PipeTabFragment extends Fragment {

    private MainActivity mainActivity;
    private VideoView videoView;
    private AlertDialog progressDialog;
    private Statistics statistics;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_pipe_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            View createButton = getView().findViewById(R.id.createButton);
            if (createButton != null) {
                createButton.setOnClickListener(new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        createVideo();
                    }
                });
            }

            videoView = getView().findViewById(R.id.videoPlayerFrame);
        }

        progressDialog = mainActivity.createProgressDialog("Creating video");
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

    public static PipeTabFragment newInstance(final MainActivity mainActivity) {
        final PipeTabFragment fragment = new PipeTabFragment();
        fragment.setMainActivity(mainActivity);
        return fragment;
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(LogMessage message) {
                Log.d(MainActivity.TAG, message.getText());
            }
        });
    }

    public void enableStatisticsCallback() {
        Config.enableStatisticsCallback(new StatisticsCallback() {

            @Override
            public void apply(final Statistics newStatistics) {
                MainActivity.addUIAction(new Callable() {

                    @Override
                    public Object call() {
                        PipeTabFragment.this.statistics = newStatistics;
                        updateProgressDialog();
                        return null;
                    }
                });
            }
        });
    }

    void startAsyncCatImageProcess(final String imagePath, final String namedPipePath) {
        AsyncCatImageTask asyncTask = new AsyncCatImageTask();
        asyncTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, imagePath, namedPipePath);
    }

    public void createVideo() {
        final File image1File = new File(mainActivity.getCacheDir(), "colosseum.jpg");
        final File image2File = new File(mainActivity.getCacheDir(), "pyramid.jpg");
        final File image3File = new File(mainActivity.getCacheDir(), "tajmahal.jpg");
        final File videoFile = getVideoFile();

        String pipe1 = Config.registerNewFFmpegPipe(mainActivity);
        String pipe2 = Config.registerNewFFmpegPipe(mainActivity);
        String pipe3 = Config.registerNewFFmpegPipe(mainActivity);

        try {

            // IF VIDEO IS PLAYING STOP PLAYBACK
            videoView.stopPlayback();

            if (videoFile.exists()) {
                videoFile.delete();
            }

            Log.d(TAG, "Testing PIPE with 'mpeg4' codec");

            showProgressDialog();

            mainActivity.resourceToFile(R.drawable.colosseum, image1File);
            mainActivity.resourceToFile(R.drawable.pyramid, image2File);
            mainActivity.resourceToFile(R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateCreateVideoWithPipesScript(pipe1, pipe2, pipe3, videoFile.getAbsolutePath());

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", ffmpegCommand));

            MainActivity.executeAsync(new ExecuteCallback() {

                @Override
                public void apply(final int returnCode, final String commandOutput) {
                    Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                    hideProgressDialog();

                    MainActivity.addUIAction(new Callable() {

                        @Override
                        public Object call() {
                            if (returnCode == RETURN_CODE_SUCCESS) {
                                Log.d(TAG, "Create completed successfully; playing video.");
                                playVideo();
                            } else {
                                Popup.show(mainActivity, "Create failed. Please check log for the details.");
                                Log.d(TAG, String.format("Create failed with rc=%d", returnCode));
                            }

                            return null;
                        }
                    });
                }
            }, ffmpegCommand);

            // START ASYNC PROCESSES AFTER INITIATING FFMPEG COMMAND
            startAsyncCatImageProcess(image1File.getAbsolutePath(), pipe1);
            startAsyncCatImageProcess(image2File.getAbsolutePath(), pipe2);
            startAsyncCatImageProcess(image3File.getAbsolutePath(), pipe3);

        } catch (IOException e) {
            Log.e(TAG, "Create video failed", e);
            Popup.show(mainActivity, "Create video failed");
        }
    }

    protected void playVideo() {
        MediaController mediaController = new MediaController(mainActivity);
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

    protected File getVideoFile() {
        return new File(mainActivity.getFilesDir(), "video.mp4");
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "Pipe Tab Activated");
        enableLogCallback();
        enableStatisticsCallback();
        Popup.show(mainActivity, Tooltip.PIPE_TEST_TOOLTIP_TEXT);
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
                textView.setText(String.format("Creating video: %% %s", completePercentage));
            }
        }
    }

    protected void hideProgressDialog() {
        progressDialog.dismiss();

        MainActivity.addUIAction(new Callable() {

            @Override
            public Object call() {
                PipeTabFragment.this.progressDialog = mainActivity.createProgressDialog("Creating video");
                return null;
            }
        });
    }

}
