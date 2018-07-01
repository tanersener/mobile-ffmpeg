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

import android.app.ProgressDialog;
import android.arch.core.util.Function;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.MediaController;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.VideoView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class SlideshowTabFragment extends Fragment {

    public static final String DEFAULT_VIDEO_CODEC = "mpeg4";

    private Context context;
    private View playButton;
    private String asyncResult;
    private EditText videoCodecText;

    public SlideshowTabFragment() {
    }

    public static SlideshowTabFragment newInstance(final Context context) {
        final SlideshowTabFragment fragment = new SlideshowTabFragment();
        fragment.setContext(context);
        return fragment;
    }

    public void setContext(Context context) {
        this.context = context;
    }

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_slideshow_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            View createButton = getView().findViewById(R.id.slideshowCreateButton);
            if (createButton != null) {
                createButton.setOnClickListener(new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        createSlideshow();
                    }
                });
            }

            final VideoView videoView = getView().findViewById(R.id.videoView);
            videoView.setBackgroundColor(Color.LTGRAY);

            videoCodecText = getView().findViewById(R.id.videoCodecText);
            videoCodecText.setText(DEFAULT_VIDEO_CODEC);
            videoCodecText.setSelection(DEFAULT_VIDEO_CODEC.length());

            // PLAY BUTTON IS DISABLED AT STARTUP
            playButton = getView().findViewById(R.id.slideshowPlayButton);
            if (playButton != null) {
                playButton.setEnabled(false);
                playButton.setOnClickListener(new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        MediaController mediaController = new MediaController(context);
                        mediaController.setAnchorView(videoView);
                        videoView.setVideoURI(Uri.parse("file://" + getVideoPath()));
                        videoView.setMediaController(mediaController);
                        videoView.requestFocus();
                        videoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {

                            @Override
                            public void onPrepared(MediaPlayer mp) {
                                videoView.setBackgroundColor(0x00000000);
                            }
                        });
                        videoView.start();
                    }
                });
            }
        }
    }

    public String getVideoPath() {
        String videoCodec = videoCodecText.getText().toString();

        final String extension;
        switch (videoCodec) {
            case "libaom-av1": {
                extension = "mkv";
            } break;
            default: {
                extension = "mp4";
            }
        }
        final String video = "slideshow." + extension;

        return new File(context.getFilesDir(), video).getAbsolutePath();
    }

    public String getCustomOptions() {
        String videoCodec = videoCodecText.getText().toString();

        switch (videoCodec) {
            case "libaom-av1": {

                // libaom support is still experimental, requires this option
                return "-strict experimental ";
            }
            default: {
                return "";
            }
        }
    }

    public void createSlideshow() {
        final String image1 = "colosseum.jpg";
        final String image2 = "pyramid.jpg";
        final String image3 = "tajmahal.jpg";

        final ProgressDialog progressDialog = ProgressDialog.show(context, "", "Creating video slideshow");

        try {
            String videoCodec = videoCodecText.getText().toString();
            if (videoCodec.trim().length() == 0) {
                videoCodec = DEFAULT_VIDEO_CODEC;
            }
            Log.i(TAG, String.format("Creating slideshow using video codec: %s", videoCodec));

            resourceToFile(R.drawable.colosseum, image1);
            resourceToFile(R.drawable.pyramid, image2);
            resourceToFile(R.drawable.tajmahal, image3);

            File file = new File(getVideoPath());
            if (file.exists()) {
                file.delete();
            }

            asyncResult = null;

            final Handler handler = new Handler();
            final Runnable runnable = new Runnable() {

                @Override
                public void run() {
                    if (asyncResult != null) {
                        String message = (asyncResult.equals("0")) ? "Slideshow created" : "Create failed with rc=" + asyncResult;

                        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();

                        handler.removeCallbacks(this);

                        if (asyncResult.equals("0")) {
                            playButton.setEnabled(true);
                        }

                    } else {
                        handler.postDelayed(this, 1000);
                    }
                }
            };
            handler.postDelayed(runnable, 1000);

            String script = Slideshow.generateScript(context.getFilesDir(), image1, image2, image3, file.getName(), videoCodec, getCustomOptions());
            Log.d(TAG, "Creating slideshow: " + script);
            MainActivity.executeAsync(new Function<Integer, Void>() {

                @Override
                public Void apply(Integer returnCode) {
                    progressDialog.cancel();
                    Log.i(TAG, "Create completed");
                    asyncResult = String.valueOf(returnCode);
                    return null;
                }
            }, script.split(" "));


        } catch (IOException e) {
            Log.e(TAG, "Creating slideshow failed", e);
            Toast.makeText(context, "Creating slideshow failed", Toast.LENGTH_SHORT).show();
        }
    }

    protected void resourceToFile(final int resourceId, final String fileName) throws IOException {
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), resourceId);

        File file = new File(context.getFilesDir(), fileName);
        if (file.exists()) {
            file.delete();
        }

        FileOutputStream outputStream = new FileOutputStream(file);
        bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
        outputStream.flush();
        outputStream.close();
        Log.d(TAG, String.format("Saved resource %d to file %s.", resourceId, fileName));
    }

}
