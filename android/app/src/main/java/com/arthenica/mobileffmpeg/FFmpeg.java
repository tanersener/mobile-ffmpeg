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
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg;

import android.util.Log;

/**
 * <p>Main class for FFmpeg operations. Provides {@link #execute(String...)} method to execute
 * FFmpeg commands.
 * <pre>
 *      int rc = FFmpeg.execute("-i", "file1.mp4", "-c:v", "libxvid", "file1.avi");
 *      Log.i(Config.TAG, String.format("Command execution %s.", (rc == 0?"completed successfully":"failed with rc=" + rc));
 * </pre>
 *
 * @author Taner Sener
 * @since v1.0
 */
public class FFmpeg {

    public static final int RETURN_CODE_SUCCESS = 0;

    public static final int RETURN_CODE_CANCEL = 255;

    static {
        Log.i(Config.TAG, "Loading mobile-ffmpeg.");

        System.loadLibrary("mobileffmpeg");

        AbiDetect.class.getName();
        Config.class.getName();

        Log.i(Config.TAG, String.format("Loaded mobile-ffmpeg-%s-%s.", AbiDetect.getAbi(), getVersion()));
    }

    /**
     * Default constructor hidden.
     */
    private FFmpeg() {
    }

    /**
     * <p>Returns FFmpeg version bundled within the library.
     *
     * @return FFmpeg version
     */
    public native static String getFFmpegVersion();

    /**
     * <p>Returns MobileFFmpeg library version.
     *
     * @return MobileFFmpeg version
     */
    public native static String getVersion();

    /**
     * <p>Synchronously executes FFmpeg with arguments provided.
     *
     * @param arguments FFmpeg command options/arguments as string array
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public native static int execute(final String[] arguments);

    /**
     * <p>Synchronously executes FFmpeg with arguments provided.
     *
     * @param arguments FFmpeg command options/arguments in one string
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public static int execute(final String arguments) {
        return execute((arguments == null) ? new String[]{""} : arguments.split(" "));
    }

    /**
     * <p>Cancels an ongoing operation.
     *
     * <p>This function does not wait for termination to complete and returns immediately.
     */
    public native static void cancel();

}
