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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.MediaController;
import android.widget.Spinner;
import android.widget.Toast;
import android.widget.VideoView;

import com.arthenica.mobileffmpeg.LogCallback;
import com.arthenica.mobileffmpeg.RunCallback;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class VideoTabFragment extends Fragment {

    private Context context;
    private View encodeButton;
    private Spinner videoCodecSpinner;

    public VideoTabFragment() {
    }

    @Override
    public View onCreateView(@NonNull final LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_video_tab, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        if (getView() != null) {
            View encodeButton = getView().findViewById(R.id.encodeButton);
            if (encodeButton != null) {
                encodeButton.setOnClickListener(new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        createVideo();
                    }
                });
            }

            final VideoView videoView = getView().findViewById(R.id.videoPlayerFrame);
            videoView.setBackgroundResource(R.color.playerColor);

            getView().findViewById(R.id.videoCodecSpinner);
        }
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        if (isVisibleToUser) {
            android.util.Log.i(MainActivity.TAG, "VIDEO TAB VIEWED");
        }
    }

    public void setContext(Context context) {
        this.context = context;
    }

    public static VideoTabFragment newInstance(final Context context) {
        final VideoTabFragment fragment = new VideoTabFragment();
        fragment.setContext(context);
        return fragment;
    }

    public void enableLogCallback() {
        com.arthenica.mobileffmpeg.Log.enableLogCallback(new LogCallback() {
            @Override
            public void apply(com.arthenica.mobileffmpeg.Log.Message message) {
                android.util.Log.i(MainActivity.TAG, message.getText());
            }
        });
    }

/*
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
*/
    public void createVideo() {
        enableLogCallback();

        final String image1 = "colosseum.jpg";
        final String image2 = "pyramid.jpg";
        final String image3 = "tajmahal.jpg";
/*
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

            String script = Video.generateCreateScript(context.getFilesDir(), image1, image2, image3, file.getName(), videoCodec, getCustomOptions());
            Log.d(TAG, "Creating slideshow: " + script);
            MainActivity.executeAsync(new RunCallback() {
                @Override
                public void apply(int returnCode) {
                    Log.d(TAG, "Create slideshow operation completed with: " + returnCode);
                    if (returnCode == 0) {
                        encodeButton.setEnabled(true);
                    }

                }
            }, script.split(" "));

        } catch (IOException e) {
            Log.e(TAG, "Creating slideshow failed", e);
            Toast.makeText(context, "Creating slideshow failed", Toast.LENGTH_SHORT).show();
        }*/
    }

    protected void playVideo() {
        final VideoView videoView = getView().findViewById(R.id.videoPlayerFrame);

        MediaController mediaController = new MediaController(context);
        mediaController.setAnchorView(videoView);
        // videoView.setVideoURI(Uri.parse("file://" + getVideoPath()));
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
