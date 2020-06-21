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
import android.widget.VideoView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.LogMessage;
import com.arthenica.mobileffmpeg.util.DialogUtil;
import com.arthenica.mobileffmpeg.util.ResourcesUtil;
import com.arthenica.mobileffmpeg.ExecuteCallback;
import com.arthenica.smartexception.java.Exceptions;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.Callable;

import static com.arthenica.mobileffmpeg.Config.RETURN_CODE_SUCCESS;
import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class VidStabTabFragment extends Fragment {

    private VideoView videoView;
    private VideoView stabilizedVideoView;
    private AlertDialog createProgressDialog;
    private AlertDialog stabilizeProgressDialog;

    public VidStabTabFragment() {
        super(R.layout.fragment_vidstab_tab);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        View stabilizeVideoButton = view.findViewById(R.id.stabilizeVideoButton);
        stabilizeVideoButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                stabilizeVideo();
            }
        });

        videoView = view.findViewById(R.id.videoPlayerFrame);
        stabilizedVideoView = view.findViewById(R.id.stabilizedVideoPlayerFrame);

        createProgressDialog = DialogUtil.createProgressDialog(requireContext(), "Creating video");
        stabilizeProgressDialog = DialogUtil.createProgressDialog(requireContext(), "Stabilizing video");
    }

    @Override
    public void onResume() {
        super.onResume();
        setActive();
    }

    public static VidStabTabFragment newInstance() {
        return new VidStabTabFragment();
    }

    public void enableLogCallback() {
        Config.enableLogCallback(new LogCallback() {

            @Override
            public void apply(LogMessage message) {
                Log.d(MainActivity.TAG, message.getText());
            }
        });
    }

    public void stabilizeVideo() {
        final File image1File = new File(requireContext().getCacheDir(), "colosseum.jpg");
        final File image2File = new File(requireContext().getCacheDir(), "pyramid.jpg");
        final File image3File = new File(requireContext().getCacheDir(), "tajmahal.jpg");
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

            Log.d(TAG, "Testing VID.STAB");

            showCreateProgressDialog();

            ResourcesUtil.resourceToFile(getResources(), R.drawable.colosseum, image1File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.pyramid, image2File);
            ResourcesUtil.resourceToFile(getResources(), R.drawable.tajmahal, image3File);

            final String ffmpegCommand = Video.generateShakingVideoScript(image1File.getAbsolutePath(), image2File.getAbsolutePath(), image3File.getAbsolutePath(), videoFile.getAbsolutePath());

            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", ffmpegCommand));

            MainActivity.executeAsync(new ExecuteCallback() {

                @Override
                public void apply(final long executionId, final int returnCode) {
                    Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                    hideCreateProgressDialog();

                    MainActivity.addUIAction(new Callable<Object>() {

                        @Override
                        public Object call() {
                            if (returnCode == RETURN_CODE_SUCCESS) {

                                Log.d(TAG, "Create completed successfully; stabilizing video.");

                                final String analyzeVideoCommand = String.format("-y -i %s -vf vidstabdetect=shakiness=10:accuracy=15:result=%s -f null -", videoFile.getAbsolutePath(), shakeResultsFile.getAbsolutePath());

                                showStabilizeProgressDialog();

                                Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", analyzeVideoCommand));

                                MainActivity.executeAsync(new ExecuteCallback() {

                                    @Override
                                    public void apply(final long executionId, final int returnCode) {
                                        Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                                        if (returnCode == RETURN_CODE_SUCCESS) {
                                            final String stabilizeVideoCommand = String.format("-y -i %s -vf vidstabtransform=smoothing=30:input=%s -c:v mpeg4 %s.", videoFile.getAbsolutePath(), shakeResultsFile.getAbsolutePath(), stabilizedVideoFile.getAbsolutePath());

                                            Log.d(TAG, String.format("FFmpeg process started with arguments\n'%s'.", stabilizeVideoCommand));

                                            MainActivity.executeAsync(new ExecuteCallback() {

                                                @Override
                                                public void apply(final long executionId, final int returnCode) {
                                                    Log.d(TAG, String.format("FFmpeg process exited with rc %d.", returnCode));

                                                    hideStabilizeProgressDialog();

                                                    MainActivity.addUIAction(new Callable<Object>() {

                                                        @Override
                                                        public Object call() {
                                                            if (returnCode == RETURN_CODE_SUCCESS) {
                                                                Log.d(TAG, "Stabilize video completed successfully; playing videos.");
                                                                playVideo();
                                                                playStabilizedVideo();
                                                            } else {
                                                                Popup.show(requireContext(), "Stabilize video failed. Please check log for the details.");
                                                                Log.d(TAG, String.format("Stabilize video failed with rc=%d.", returnCode));
                                                            }

                                                            return null;
                                                        }
                                                    });
                                                }
                                            }, stabilizeVideoCommand);

                                        } else {
                                            hideStabilizeProgressDialog();
                                            Popup.show(requireContext(), "Stabilize video failed. Please check log for the details.");
                                            Log.d(TAG, String.format("Stabilize video failed with rc=%d.", returnCode));
                                        }
                                    }
                                }, analyzeVideoCommand);

                            } else {
                                Popup.show(requireContext(), "Create video failed. Please check log for the details.");
                                Log.d(TAG, String.format("Create failed with rc=%d.", returnCode));
                            }

                            return null;
                        }
                    });
                }
            }, ffmpegCommand);

        } catch (IOException e) {
            Log.e(TAG, String.format("Stabilize video failed %s.", Exceptions.getStackTraceString(e)));
            Popup.show(requireContext(), "Stabilize video failed");
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

    protected void playStabilizedVideo() {
        MediaController mediaController = new MediaController(requireContext());
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
        return new File(requireContext().getCacheDir(), "transforms.trf");
    }

    public File getVideoFile() {
        return new File(requireContext().getFilesDir(), "video.mp4");
    }

    public File getStabilizedVideoFile() {
        return new File(requireContext().getFilesDir(), "video-stabilized.mp4");
    }

    public void setActive() {
        Log.i(MainActivity.TAG, "VidStab Tab Activated");
        enableLogCallback();
        Popup.show(requireContext(), Tooltip.VIDSTAB_TEST_TOOLTIP_TEXT);
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
