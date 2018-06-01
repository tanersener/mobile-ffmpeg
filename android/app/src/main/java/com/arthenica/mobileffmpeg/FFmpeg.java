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
 * <p>Base class for FFmpeg operations.
 *
 * <p>Note that before terminating {@link #shutdown()} method must be called.
 *
 * @author Taner Sener
 */
public class FFmpeg {

    static {
        final Abi abi = Abi.from(AbiDetect.getAbi());
        String abiName = abi.getValue();

        Log.enableCollectingStdOutErr();

        android.util.Log.i(Log.TAG, "Loading mobile-ffmpeg.");

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
                abiName = Abi.ABI_ARMV7A.getValue();
            }
        }

        if (!nativeLibraryLoaded) {
            System.loadLibrary("mobileffmpeg");

            android.util.Log.i(Log.TAG, String.format("Loaded mobile-ffmpeg-%s-%s.", abiName, getVersion()));
        }
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

    /**
     * <p>Disables collecting stdout and stderr.
     */
    public static void shutdown() {
        Log.disableCollectingStdOutErr();
    }

}
