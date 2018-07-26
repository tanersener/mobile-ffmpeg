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

/**
 * <p>Main class for FFmpeg operations. Provides {@link #execute(String...)} method to execute
 * FFmpeg commands.
 * <pre>
 *      int rc = FFmpeg.execute("-i", "file1.mp4", "-c:v", "libxvid", "file1.avi");
 *      Log.i(Log.TAG, String.format("Command execution %s.", (rc == 0?"completed successfully":"failed with rc=" + rc));
 * </pre>
 *
 * @author Taner Sener
 * @since v1.0
 */
public class FFmpeg {

    static {
        android.util.Log.i(Log.TAG, "Loading mobile-ffmpeg.");

        final Abi abi = Abi.from(AbiDetect.getAbi());
        String abiName = abi.getName();

        Log.enableRedirection();

        /*
         * NEON supported arm-v7a library has a different name
         */
        boolean nativeLibraryLoaded = false;
        if (abi == Abi.ABI_ARMV7A_NEON) {
            try {
                System.loadLibrary("mobileffmpeg-armv7a-neon");
                android.util.Log.i(Log.TAG, String.format("Loaded mobile-ffmpeg-%s-%s.", abiName, getVersion()));
                nativeLibraryLoaded = true;
            } catch (UnsatisfiedLinkError e) {
                android.util.Log.i(Log.TAG, "NEON supported armeabi-v7a library not found. Loading default armeabi-v7a library.", e);
                abiName = Abi.ABI_ARMV7A.getName();
            }
        }

        if (!nativeLibraryLoaded) {
            System.loadLibrary("mobileffmpeg");

            android.util.Log.i(Log.TAG, String.format("Loaded mobile-ffmpeg-%s-%s.", abiName, getVersion()));
        }
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
     * @param arguments FFmpeg command options/arguments
     * @return zero on successful execution, non-zero on error
     */
    public native static int execute(final String ... arguments);

}
