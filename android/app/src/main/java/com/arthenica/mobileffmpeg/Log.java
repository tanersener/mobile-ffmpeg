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

/**
 * <p>This class is used to process FFmpeg logs.
 *
 * <p>By default it redirects FFmpeg output to Logcat. Alternatively, it is possible not to print
 * messages to Logcat and pass them to a {@link LogCallback} function. This function can decide
 * whether to print these logs, show them inside another container or ignore them.
 *
 * @author Taner Sener
 * @since v1.0
 */
public class Log {

    /**
     * Defines tag used for logging.
     */
    public static final String TAG = "mobile-ffmpeg";

    public static final class Message {
        private final Level level;
        private final String text;

        public Message(final Level level, final String text) {
            this.level = level;
            this.text = text;
        }

        public Level getLevel() {
            return level;
        }

        public String getText() {
            return text;
        }
    }

    private static LogCallback callbackFunction;

    private static Level activeLogLevel;

    static {
        System.loadLibrary("ffmpeglog");

        /* ALL LIBRARIES LOADED AT STARTUP */
        AbiDetect.class.getName();
        FFmpeg.class.getName();

        /* NATIVE LOG LEVEL IS RECEIVED ONLY ON STARTUP */
        activeLogLevel = Level.from(getNativeLevel());
    }

    /**
     * Default constructor hidden.
     */
    private Log() {
    }

    /**
     * <p>Enables log redirection.
     */
    public static void enableRedirection() {
        enableNativeRedirection();
    }

    /**
     * <p>Disables log redirection.
     */
    public static void disableRedirection() {
        disableNativeRedirection();
    }

    /**
     * Returns log level.
     *
     * @return log level
     */
    public static Level getLevel() {
        return activeLogLevel;
    }

    /**
     * Sets log level.
     *
     * @param level log level
     */
    public static void setLevel(final Level level) {
        if (level != null) {
            activeLogLevel = level;
            setNativeLevel(level.getValue());
        }
    }

    /**
     * <p>Sets a callback function to redirect FFmpeg logs.
     *
     * @param newLogCallback new log callback function
     */
    public static void enableLogCallback(final LogCallback newLogCallback) {
        callbackFunction = newLogCallback;
    }

    /**
     * <p>Log redirection method called by JNI/native part.
     *
     * @param levelValue log level as defined in {@link Level}
     * @param logMessage redirected log message
     */
    private static void log(final int levelValue, final byte[] logMessage) {
        final Level level = Level.from(levelValue);
        final String text = new String(logMessage);

        if (activeLogLevel == Level.AV_LOG_QUIET || levelValue > activeLogLevel.getValue()) {
            // LOG NEITHER PRINTED NOR FORWARDED
            return;
        }

        if (callbackFunction != null) {
            callbackFunction.apply(new Message(level, text));
        } else {
            switch (level) {
                case AV_LOG_QUIET: {
                    // PRINT NO OUTPUT
                }
                break;
                case AV_LOG_TRACE:
                case AV_LOG_DEBUG: {
                    android.util.Log.d(TAG, text);
                }
                break;
                case AV_LOG_VERBOSE: {
                    android.util.Log.v(TAG, text);
                }
                break;
                case AV_LOG_INFO: {
                    android.util.Log.i(TAG, text);
                }
                break;
                case AV_LOG_WARNING: {
                    android.util.Log.w(TAG, text);
                }
                break;
                case AV_LOG_ERROR:
                case AV_LOG_FATAL:
                case AV_LOG_PANIC: {
                    android.util.Log.e(TAG, text);
                }
                break;
                default: {
                    android.util.Log.v(TAG, text);
                }
                break;
            }
        }
    }

    /**
     * <p>Enables native log redirection.
     */
    private static native void enableNativeRedirection();

    /**
     * <p>Disables native log redirection
     */
    private static native void disableNativeRedirection();

    /**
     * Sets native log level
     *
     * @param level log level
     */
    private static native void setNativeLevel(int level);

    /**
     * Returns native log level.
     *
     * @return log level
     */
    private static native int getNativeLevel();

}
