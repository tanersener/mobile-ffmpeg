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
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.MediaController;
import android.widget.VideoView;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.ExecuteCallback;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.FFmpeg.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class VidStabTabFragment extends Fragment {

    private MainActivity mainActivity;
    private VideoView videoView;
    private VideoView stabilizedVideoView;
    private AlertDialog createProgressDialog;
    private AlertDialog stabilizeProgressDialog;

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_vidstab_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            View stabilizeVideoButton = getView().findViewById(R.id.stabilizeVideoButton);
            stabilizeVideoButton.setOnClickListener(new View.OnClickListener() {

                @Override
                public void onClick(View v) {
                    stabilizeVideo();
                }
            });

            videoView = getView().findViewById(R.id.videoPlayerFrame);
            stabilizedVideoView = getView().findViewById(R.id.stabilizedVideoPlayerFrame);
        }

        createProgressDialog = mainActivity.createProgressDialog("Creating video");
        stabilizeProgressDialog = mainActivity.createProgressDialog("Stabilizing video");
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

    public static VidStabTabFragment newInstance(final MainActivity mainActivity) {
        final VidStabTabFragment fragment = new VidStabTabFragment();
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

    public void stabilizeVideo() {
        final File image1File = new File(mainActivity.getCacheDir(), "colosseum.jpg");
        final File image2File = new File(mainActivity.getCacheDir(), "pyramid.jpg");
        final File image3File = new File(mainActivity.getCacheDir(), "tajmahal.jpg");
        final File shakeResultsFile = getShakeResultsFile();
        final File videoFile = getVideoFile();
        final File stabilizedVideoFile = getStabilizedVideoFile();

        try {

            // IF VIDEO IS PLAYING STOP PLAYBACK
            videoView.stopPlayback();
            stabilizedVideoView.stopPlayback();

            if (shakeResultsFile.exists()) {
                shakeResultsFile.delete();
            }
            if (videoFile.exists()) {
                videoFile.delete();
            }
            if (stabilizedVideoFile.exists()) {
                stabilizedVideoFile.delete();
            }

            android.util.Log.d(TAG, "Testing VID.STAB");

            showCreateProgressDialog();

            mainActivity.resourceToFile(R.drawable.colosseum, image1File);
            mainActivity.resourceToFile(R.drawable.pyramid, image2File);
            mainActivity.resourceToFile(R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateShakingVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath());

            android.util.Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", ffmpegCommand));

            MainActivity.executeAsync(new ExecuteCallback() {

                @Override
                public void apply(final int returnCode, final String commandOutput) {
                    android.util.Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                    hideCreateProgressDialog();

                    MainActivity.addUIAction(new Callable() {

                        @Override
                        public Object call() {
                            if (returnCode == RETURN_CODE_SUCCESS) {

                                android.util.Log.d(TAG, "Create completed successfully; stabilizing video.");

                                final String analyzeVideoCommand = String.format("-y -i %s -vf vidstabdetect=shakiness=10:accuracy=15:result=%s -f null -", videoFile.getAbsolutePath(), shakeResultsFile.getAbsolutePath());

                                showStabilizeProgressDialog();

                                android.util.Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", analyzeVideoCommand));

                                MainActivity.executeAsync(new ExecuteCallback() {

                                    @Override
                                    public void apply(final int returnCode, final String commandOutput) {
                                        android.util.Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                                        if (returnCode == RETURN_CODE_SUCCESS) {
                                            final String stabilizeVideoCommand = String.format("-y -i %s -vf vidstabtransform=smoothing=30:input=%s -c:v mpeg4 %s", videoFile.getAbsolutePath(), shakeResultsFile.getAbsolutePath(), stabilizedVideoFile.getAbsolutePath());

                                            android.util.Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'", stabilizeVideoCommand));

                                            MainActivity.executeAsync(new ExecuteCallback() {

                                                @Override
                                                public void apply(final int returnCode, final String commandOutput) {
                                                    android.util.Log.d(TAG, String.format("FFmpeg process exited with rc %d", returnCode));

                                                    hideStabilizeProgressDialog();

                                                    MainActivity.addUIAction(new Callable() {

                                                        @Override
                                                        public Object call() {
                                                            if (returnCode == RETURN_CODE_SUCCESS) {
                                                                android.util.Log.d(TAG, "Stabilize video completed successfully; playing videos.");
                                                                playVideo();
                                                                playStabilizedVideo();
                                                            } else {
                                                                Popup.show(mainActivity, "Stabilize video failed. Please check log for the details.");
                                                                android.util.Log.d(TAG, String.format("Stabilize video failed with rc=%d", returnCode));
                                                            }

                                                            return null;
                                                        }
                                                    });
                                                }
                                            }, stabilizeVideoCommand);

                                        } else {
                                            hideStabilizeProgressDialog();
                                            Popup.show(mainActivity, "Stabilize video failed. Please check log for the details.");
                                            android.util.Log.d(TAG, String.format("Stabilize video failed with rc=%d", returnCode));
                                        }
                                    }
                                }, analyzeVideoCommand);

                            } else {
                                Popup.show(mainActivity, "Create video failed. Please check log for the details.");
                                android.util.Log.d(TAG, String.format("Create failed with rc=%d", returnCode));
                            }

                            return null;
                        }
                    });
                }
            }, ffmpegCommand);

        } catch (IOException e) {
            android.util.Log.e(TAG, "Stabilize video failed", e);
            Popup.show(mainActivity, "Stabilize video failed");
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

    protected void playStabilizedVideo() {
        MediaController mediaController = new MediaController(mainActivity);
        mediaController.setAnchorView(stabilizedVideoView);
        stabilizedVideoView.setVideoURI(Uri.parse("file://" + getStabilizedVideoFile().getAbsolutePath()));
        stabilizedVideoView.setMediaController(mediaController);
        stabilizedVideoView.requestFocus();
        stabilizedVideoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {

            @Override
            public void onPrepared(MediaPlayer mp) {
                stabilizedVideoView.setBackgroundColor(0x00000000);
            }
        });
        stabilizedVideoView.setOnErrorListener(new MediaPlayer.OnErrorListener() {

            @Override
            public boolean onError(MediaPlayer mp, int what, int extra) {
                stabilizedVideoView.stopPlayback();
                return false;
            }
        });
        stabilizedVideoView.start();
    }

    public File getShakeResultsFile() {
        return new File(mainActivity.getCacheDir(), "transforms.trf");
    }

    public File getVideoFile() {
        return new File(mainActivity.getFilesDir(), "video.mp4");
    }

    public File getStabilizedVideoFile() {
        return new File(mainActivity.getFilesDir(), "video-stabilized.mp4");
    }

    public void setActive() {
        android.util.Log.i(MainActivity.TAG, "VidStab Tab Activated");
        enableLogCallback();
        Popup.show(mainActivity, Tooltip.VIDSTAB_TEST_TOOLTIP_TEXT);
    }

    protected void showCreateProgressDialog() {
        createProgressDialog.show();
    }

    protected void hideCreateProgressDialog() {
        createProgressDialog.dismiss();
    }

    protected void showStabilizeProgressDialog() {
        stabilizeProgressDialog.show();
    }

    protected void hideStabilizeProgressDialog() {
        stabilizeProgressDialog.dismiss();
    }

}
