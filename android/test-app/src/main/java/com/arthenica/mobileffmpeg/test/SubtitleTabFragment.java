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
import android.util.Log;
import android.view.View;
import android.widget.MediaController;
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

import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_CANCEL;
import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class SubtitleTabFragment extends Fragment {

    private enum State {
        IDLE,
        CREATING,
        BURNING
    }

    private VideoView videoView;
    private AlertDialog createProgressDialog;
    private AlertDialog burnProgressDialog;
    private Statistics statistics;
    private State state;
    private Long executionId;

    public SubtitleTabFragment() {
        super(R.layout.fragment_subtitle_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        View burnSubtitlesButton = view.findViewById(R.id.burnSubtitlesButton);
        burnSubtitlesButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                burnSubtitles();
            }
        });

        videoView = view.findViewById(R.id.videoPlayerFrame);

        state = State.IDLE;
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static SubtitleTabFragment newInstance() {
        return new SubtitleTabFragment();
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
                        SubtitleTabFragment.this.statistics = newStatistics;
                        updateProgressDialog();
                        return null;
                    }
                });
            }
        });
    }

    public void burnSubtitles() {
        final File image1File = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File image2File = new File(requireContext().getCacheDir(), "pyramid.jpg");
        final File image3File = new File(requireContext().getCacheDir(), "tajmahal.jpg");
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

            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, image1File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.pyramid, image2File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.tajmahal, image3File);
            ResourcesUtil.rawResourceToFile(getResources(), R.raw.subtitle, getSubtitleFile());

            final String ffmpegCommand = Video.generateEncodeVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath(), "mpeg4", "");

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

            state = State.CREATING;

            executionId = FFmpeg.executeAsync(ffmpegCommand, new ExecuteCallback() {

                @Override
                public void apply(final long executionId, final int returnCode) {
                    Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                    hideCreateProgressDialog();

                    if (returnCode == RETURN_CODE_SUCCESS) {

                        MainActivity.addUIAction(new Callable<Object>() {

                            @Override
                            public Object call() {

                                Log.d(TAG, "Create completed successfully; burning subtitles.");

                                String burnSubtitlesCommand = String.format("-y -i %s -vf subtitles=%s:force_style='FontName=MyFontName' -c:v mpeg4 %s", videoFile.getAbsolutePath(), getSubtitleFile().getAbsolutePath(), videoWithSubtitlesFile.getAbsolutePath());

                                showBurnProgressDialog();

                                Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", burnSubtitlesCommand));

                                state = State.BURNING;

                                FFmpeg.executeAsync(burnSubtitlesCommand, new ExecuteCallback() {

                                    @Override
                                    public void apply(final long executionId, final int returnCode) {
                                        Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                                        hideBurnProgressDialog();

                                        MainActivity.addUIAction(new Callable<Object>() {

                                            @Override
                                            public Object call() {
                                                if (returnCode == RETURN_CODE_SUCCESS) {
                                                    Log.d(TAG, "Burn subtitles completed successfully; playing video.");
                                                    playVideo();
                                                } else if (returnCode == RETURN_CODE_CANCEL) {
                                                    Popup.show(requireContext(), "Burn subtitles operation cancelled.");
                                                    Log.e(TAG, "Burn subtitles operation cancelled");
                                                } else {
                                                    Popup.show(requireContext(), "Burn subtitles failed. Please check log for the details.");
                                                    Log.e(TAG, String.format("Burn subtitles failed with rc=%d.", returnCode));
                                                }

                                                return null;
                                            }
                                        });
                                    }
                                });

                                return null;
                            }
                        });
                    }
                }
            });

            Log.d(TAG, String.format("FFmpeg started execution id: %d.", executionId));

        } catch (IOException e) {
            Log.e(TAG, String.format("Burn subtitles failed %s.", Exceptions.getStackTraceString(e)));
            Popup.show(requireContext(), "Burn subtitles failed");
        }
    }

    protected void playVideo() {
        MediaController mediaController = new MediaController(requireContext());
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
        return new File(requireContext().getCacheDir(), "subtitle.srt");
    }

    public File getVideoFile() {
        return new File(requireContext().getFilesDir(), "video.mp4");
    }

    public File getVideoWithSubtitlesFile() {
        return new File(requireContext().getFilesDir(), "video-with-subtitles.mp4");
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "Subtitle Tab Activated");
        enableLogCallback();
        enableStatisticsCallback();
        Popup.show(requireContext(), getString(R.string.subtitle_test_tooltip_text));
    }

    protected void showCreateProgressDialog() {

        // CLEAN STATISTICS
        statistics = null;
        Config.resetStatistics();

        createProgressDialog = DialogUtil.createCancellableProgressDialog(requireContext(), "Creating video", new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                if (executionId != null) {
                    Log.d(TAG, String.format("Cancelling FFmpeg execution with executionId %d.", executionId));
                    FFmpeg.cancel(executionId);
                }
            }
        });
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
                textView.setText(String.format("Creating video: %% %s.", completePercentage));
            }
        } else if (state == State.BURNING) {
            TextView textView = burnProgressDialog.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(String.format("Burning subtitles: %% %s.", completePercentage));
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

        burnProgressDialog = DialogUtil.createCancellableProgressDialog(requireContext(), "Burning subtitles", new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FFmpeg.cancel();
            }
        });
        burnProgressDialog.show();
    }

    protected void hideBurnProgressDialog() {
        burnProgressDialog.dismiss();
    }

}
