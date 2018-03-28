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

public class Log {

    public static final String TAG = "mobile-ffmpeg";

    protected static boolean enableCollectLogMode;

    protected static StringBuilder collectedLog;

    static {
        enableCollectLogMode = false;
        collectedLog = new StringBuilder();

        System.loadLibrary("ffmpeglog");
    }

    public static void enableCollectingStdOutErr() {
        startNativeCollector();
    }

    public static void disableCollectingStdOutErr() {
        stopNativeCollector();
    }

    public static void enableCollectLogMode() {
        enableCollectLogMode = true;
    }

    public static void cleanCollectedLog() {
        collectedLog = new StringBuilder();
    }

    public static String getCollectedLog() {
        return collectedLog.toString();
    }

    public static void log(final String logMessage) {
        if (enableCollectLogMode) {
            collectedLog.append(logMessage);
        } else {
            android.util.Log.d(TAG, logMessage);
        }
    }

    public static native int startNativeCollector();

    public static native int stopNativeCollector();

}
