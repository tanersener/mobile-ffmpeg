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

public class Log {

    public static final String TAG = "mobile-ffmpeg";

    private static Function<byte[], Void> callbackFunction;

    static {
        System.loadLibrary("ffmpeglog");
    }

    public static void enableCollectingStdOutErr() {
        startNativeCollector();
    }

    public static void disableCollectingStdOutErr() {
        stopNativeCollector();
    }

    public static void enableCallbackFunction(final Function<byte[], Void> newCallbackFunction) {
        callbackFunction = newCallbackFunction;
    }

    public static void log(final byte[] logMessage) {
        if (callbackFunction != null) {
            callbackFunction.apply(logMessage);
        } else {
            android.util.Log.d(TAG, new String(logMessage));
        }
    }

    public static native int startNativeCollector();

    public static native int stopNativeCollector();

}
