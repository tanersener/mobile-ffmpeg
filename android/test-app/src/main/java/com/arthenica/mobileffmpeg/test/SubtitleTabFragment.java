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
import android.os.Environment;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.MediaController;
import android.widget.TextView;
import android.widget.VideoView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.RunCallback;
import com.arthenica.mobileffmpeg.Statistics;
import com.arthenica.mobileffmpeg.StatisticsCallback;

import java.io.File;
import java.io.IOException;
import java.math.BigDecimal;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.FFmpeg.RETURN_CODE_CANCEL;
import static com.arthenica.mobileffmpeg.FFmpeg.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class SubtitleTabFragment extends Fragment {

    private enum State {
        IDLE,
        CREATING,
        BURNING
    }

    private MainActivity mainActivity;
    private VideoView videoView;
    private AlertDialog createProgressDialog;
    private AlertDialog burnProgressDialog;
    private Statistics statistics;
    private State state;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_subtitle_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            View burnSubtitlesButton = getView().findViewById(R.id.burnSubtitlesButton);
            burnSubtitlesButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    burnSubtitles();
                }
            });

            videoView = getView().findViewById(R.id.videoPlayerFrame);
        }

        createProgressDialog = mainActivity.createCancellableProgressDialog("Creating video", new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                FFmpeg.cancel();
            }
        });
        burnProgressDialog = mainActivity.createCancellableProgressDialog("Burning subtitles", new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FFmpeg.cancel();
            }
        });

        state = State.IDLE;
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

    public static SubtitleTabFragment newInstance(final MainActivity mainActivity) {
        final SubtitleTabFragment fragment = new SubtitleTabFragment();
        fragment.setMainActivity(mainActivity);
        return fragment;
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
                MainActivity.addUIAction(new Callable() {

                    @Override
                    public Object call() {
                        SubtitleTabFragment.this.statistics = newStatistics;
                        updateProgressDialog();
                        return null;
                    }
                });
            }
        });
    }

    public void burnSubtitles() {
        final File image1File = new File(mainActivity.getCacheDir(), "colosseum.jpg");
        final File image2File = new File(mainActivity.getCacheDir(), "pyramid.jpg");
        final File image3File = new File(mainActivity.getCacheDir(), "tajmahal.jpg");
        final File videoFile = getVideoFile();
        final File videoWithSubtitlesFile = getVideoWithSubtitlesFile();

        try {

            // IF VIDEO IS PLAYING STOP PLAYBACK
            videoView.stopPlayback();

            if (videoFile.exists()) {
                videoFile.delete();
            }

            if (videoWithSubtitlesFile.exists()) {
                videoWithSubtitlesFile.delete();
            }

            Log.d(TAG, "Testing SUBTITLE burning");

            showCreateProgressDialog();

            mainActivity.resourceToFile(R.drawable.colosseum, image1File);
            mainActivity.resourceToFile(R.drawable.pyramid, image2File);
            mainActivity.resourceToFile(R.drawable.tajmahal, image3File);
            mainActivity.rawResourceToFile(R.raw.subtitle, getSubtitleFile());

            final String ffmpegCommand = Video.generateEncodeVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath(), "mpeg4", "");

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", ffmpegCommand));

            state = State.CREATING;

            MainActivity.executeAsync(new RunCallback() {

                @Override
                public void apply(final int returnCode) {
                    Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                    state = State.IDLE;

                    hideCreateProgressDialog();

                    MainActivity.addUIAction(new Callable() {

                        @Override
                        public Object call() {
                            if (returnCode == RETURN_CODE_SUCCESS) {

                                Log.d(TAG, "Create completed successfully; burning subtitles.");

                                String burnSubtitlesCommand = String.format("-y -i %s -vf subtitles=%s:force_style='FontName=MyFontName' %s", videoFile.getAbsolutePath(), getSubtitleFile().getAbsolutePath(), videoWithSubtitlesFile.getAbsolutePath());

                                showBurnProgressDialog();

                                Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", burnSubtitlesCommand));

                                state = State.BURNING;

                                MainActivity.executeAsync(new RunCallback() {

                                    @Override
                                    public void apply(final int returnCode) {
                                        Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                                        state = State.IDLE;

                                        hideBurnProgressDialog();

                                        MainActivity.addUIAction(new Callable() {

                                            @Override
                                            public Object call() {
                                                if (returnCode == RETURN_CODE_SUCCESS) {
                                                    Log.d(TAG, "Burn subtitles completed successfully; playing video.");
                                                    playVideo();
                                                } else if (returnCode == RETURN_CODE_CANCEL) {
                                                    Popup.show(mainActivity, "Burn subtitles operation cancelled.");
                                                    Log.d(TAG, "Burn subtitles operation cancelled");
                                                } else {
                                                    Popup.show(mainActivity, "Burn subtitles failed. Please check log for the details.");
                                                    Log.d(TAG, String.format("Burn subtitles failed with rc=%d", returnCode));
                                                }

                                                return null;
                                            }
                                        });
                                    }
                                }, burnSubtitlesCommand);

                            } else if (returnCode == RETURN_CODE_CANCEL) {
                                Popup.show(mainActivity, "Create operation cancelled.");
                                Log.d(TAG, "Create operation cancelled");
                            } else {
                                Popup.show(mainActivity, "Create video failed. Please check log for the details.");
                                Log.d(TAG, String.format("Create failed with rc=%d", returnCode));
                            }

                            return null;
                        }
                    });
                }
            }, ffmpegCommand);

        } catch (IOException e) {
            Log.e(TAG, "Burn subtitles failed", e);
            Popup.show(mainActivity, "Burn subtitles failed");
        }
    }

    protected void playVideo() {
        MediaController mediaController = new MediaController(mainActivity);
        mediaController.setAnchorView(videoView);
        videoView.setVideoURI(Uri.parse("file://" + getVideoWithSubtitlesFile().getAbsolutePath()));
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

    public File getSubtitleFile() {
        return new File(mainActivity.getCacheDir(), "subtitle.srt");
    }

    public File getVideoFile() {
        return new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "video.mp4");
    }

    public File getVideoWithSubtitlesFile() {
        return new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "video-with-subtitles.mp4");
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "Subtitle Tab Activated");
        enableLogCallback();
        enableStatisticsCallback();
        Popup.show(mainActivity, Tooltip.SUBTITLE_TEST_TOOLTIP_TEXT);
    }

    protected void showCreateProgressDialog() {

        // CLEAN STATISTICS
        statistics = null;
        Config.resetStatistics();

        createProgressDialog.show();
    }

    protected void updateProgressDialog() {
        if (statistics == null) {
            return;
        }

        int timeInMilliseconds = this.statistics.getTime();
        int totalVideoDuration = 9000;

        String completePercentage = new BigDecimal(timeInMilliseconds).multiply(new BigDecimal(100)).divide(new BigDecimal(totalVideoDuration), 0, BigDecimal.ROUND_HALF_UP).toString();

        if (state == State.CREATING) {
            TextView textView = createProgressDialog.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(String.format("Creating video: %% %s", completePercentage));
            }
        } else if (state == State.BURNING) {
            TextView textView = burnProgressDialog.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(String.format("Burning subtitles: %% %s", completePercentage));
            }
        }

    }

    protected void hideCreateProgressDialog() {
        createProgressDialog.dismiss();
    }

    protected void showBurnProgressDialog() {

        // CLEAN STATISTICS
        statistics = null;
        Config.resetStatistics();

        burnProgressDialog.show();
    }

    protected void hideBurnProgressDialog() {
        burnProgressDialog.dismiss();
    }

}
