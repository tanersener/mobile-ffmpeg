/*
 * Simple DirectMedia Layer
 * Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * CHANGES 07.2020
 * - SDLAudioHandler renamed as GenericAudioHandler
 */

package com.arthenica.mobileffmpeg.player;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Build;
import android.util.Log;

import static com.arthenica.mobileffmpeg.Config.TAG;

public class GenericAudioHandler implements AudioHandler {
    protected AudioTrack audioTrack;
    protected AudioRecord audioRecord;

    public void initialize() {
        audioTrack = null;
        audioRecord = null;
    }

    public int audioOpen(final int sampleRate, final boolean is16Bit, final boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);

        Log.v(TAG, String.format("AudioHandler audio: wanted %s %s %skHz, %d frames buffer.", isStereo ? "stereo" : "mono", is16Bit ? "16-bit" : "8-bit", sampleRate / 1000f, desiredFrames));

        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);

        if (audioTrack == null) {
            audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                    channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);

            // Instantiating AudioTrack can "succeed" without an exception and the track may still be invalid
            // Ref: https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/media/java/android/media/AudioTrack.java
            // Ref: http://developer.android.com/reference/android/media/AudioTrack.html#getState()

            if (audioTrack.getState() != AudioTrack.STATE_INITIALIZED) {
                Log.e(TAG, "AudioHandler failed during initialization of AudioTrack.");
                audioTrack = null;
                return -1;
            }

            audioTrack.play();
        }

        Log.v(TAG, String.format("AudioHandler audio: got %s %s %skHz, %d frames buffer.", (audioTrack.getChannelCount() >= 2) ? "stereo" : "mono", (audioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit", audioTrack.getSampleRate() / 1000f, desiredFrames));

        return 0;
    }

    public void audioWriteShortBuffer(final short[] buffer) {
        if (audioTrack == null) {
            Log.e(TAG, "AudioHandler attempted to make audio call with uninitialized audio!");
            return;
        }

        for (int i = 0; i < buffer.length; ) {
            int result = audioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w(TAG, "AudioHandler audio: error return from write(short).");
                return;
            }
        }
    }

    public void audioWriteByteBuffer(final byte[] buffer) {
        if (audioTrack == null) {
            Log.e(TAG, "AudioHandler attempted to make audio call with uninitialized audio!");
            return;
        }

        for (int i = 0; i < buffer.length; ) {
            int result = audioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch (InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w(TAG, "AudioHandler audio: error return from write(byte).");
                return;
            }
        }
    }

    public int captureOpen(final int sampleRate, final boolean is16Bit, final boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_IN_STEREO : AudioFormat.CHANNEL_IN_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);

        Log.v(TAG, String.format("AudioHandler capture: wanted %s %s %skHz, %d frames buffer.", isStereo ? "stereo" : "mono", is16Bit ? "16-bit" : "8-bit", sampleRate / 1000f, desiredFrames));

        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);

        if (audioRecord == null) {
            audioRecord = new AudioRecord(MediaRecorder.AudioSource.DEFAULT, sampleRate, channelConfig, audioFormat, desiredFrames * frameSize);

            // see notes about AudioTrack state in audioOpen(), above. Probably also applies here.
            if (audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "AudioHandler failed during initialization of AudioRecord.");
                audioRecord.release();
                audioRecord = null;
                return -1;
            }

            audioRecord.startRecording();
        }

        Log.v(TAG, String.format("AudioHandler capture: got %s %s %skHz, %d frames buffer.", (audioRecord.getChannelCount() >= 2) ? "stereo" : "mono", (audioRecord.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit", audioRecord.getSampleRate() / 1000f, desiredFrames));

        return 0;
    }

    public int captureReadShortBuffer(final short[] buffer, final boolean blocking) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return audioRecord.read(buffer, 0, buffer.length, blocking ? AudioRecord.READ_BLOCKING : AudioRecord.READ_NON_BLOCKING);
        } else {
            return audioRecord.read(buffer, 0, buffer.length);
        }
    }

    public int captureReadByteBuffer(final byte[] buffer, final boolean blocking) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return audioRecord.read(buffer, 0, buffer.length, blocking ? AudioRecord.READ_BLOCKING : AudioRecord.READ_NON_BLOCKING);
        } else {
            return audioRecord.read(buffer, 0, buffer.length);
        }
    }

    public void audioClose() {
        if (audioTrack != null) {
            audioTrack.stop();
            audioTrack.release();
            audioTrack = null;
        }
    }

    public void captureClose() {
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null;
        }
    }

}
