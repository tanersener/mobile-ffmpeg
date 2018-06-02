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

package com.arthenica.mobileffmpeg;

import android.arch.core.util.Function;

/**
 * <p>This class is used to process stdout and stderr logs from native libraries.
 *
 * <p>By default stdout and stderr is redirected to <code>/dev/null</code> in Android. This class
 * redirects these streams to Logcat in order to view logs printed by FFmpeg native libraries.
 *
 * <p>Alternatively, it is possible not to print messages to Logcat and pass them to a callback
 * function. This function can decide whether to print these logs or ignore them according to its
 * own rules.
 *
 * @author Taner Sener
 * @since v1.0
 */
public class Log {

    /**
     * Defines tag used for logging.
     */
    public static final String TAG = "mobile-ffmpeg";

    private static Function<byte[], Void> callbackFunction;

    static {
        System.loadLibrary("ffmpeglog");
    }

    /**
     * Default constructor hidden.
     */
    private Log() {
    }

    /**
     * <p>Enables redirecting stdout and stderr.
     */
    public static void enableCollectingStdOutErr() {
        startNativeCollector();
    }

    /**
     * <p>Disables redirecting stdout and stderr.
     */
    public static void disableCollectingStdOutErr() {
        stopNativeCollector();
    }

    /**
     * <p>Sets a callback function to receive logs from FFmpeg native libraries.
     *
     * @param newCallbackFunction callback to receive logs
     */
    public static void enableCallbackFunction(final Function<byte[], Void> newCallbackFunction) {
        callbackFunction = newCallbackFunction;
    }

    /**
     * <p>Main method called by JNI part to redirect log messages. It is not designed to be called
     * manually by Java classes.
     *
     * @param logMessage log message
     */
    public static void log(final byte[] logMessage) {
        if (callbackFunction != null) {
            callbackFunction.apply(logMessage);
        } else {
            android.util.Log.d(TAG, new String(logMessage));
        }
    }

    /**
     * <p>Starts native log collector.
     *
     * @return zero on success, non-zero if an error occurs
     */
    public static native int startNativeCollector();

    /**
     * <p>Stops native log collector.
     *
     * @return zero on success, non-zero if an error occurs
     */
    public static native int stopNativeCollector();

}
