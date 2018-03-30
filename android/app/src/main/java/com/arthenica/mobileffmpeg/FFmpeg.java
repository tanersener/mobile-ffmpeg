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

import android.arch.core.util.Function;

import com.arthenica.mobileffmpeg.util.AsynchronousTaskService;

import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

/**
 * <p>Base class for FFmpeg operations.
 *
 * <p>Note that before terminating {@link #shutdown()} method must be called.
 *
 * @author Taner Sener
 */
public class FFmpeg {

    protected static AsynchronousTaskService asynchronousTaskService;

    static {
        Abi abi = Abi.from(AbiDetect.getAbi());

        Log.enableCollectingStdOutErr();

        /*
         * NEON supported arm-v7a library has a different name
         */
        if (abi == Abi.ABI_ARMV7A_NEON) {
            System.loadLibrary("mobileffmpeg-armv7a-neon");
        } else {
            System.loadLibrary("mobileffmpeg");
        }

        android.util.Log.i(Log.TAG, "Loaded mobile-ffmpeg-" + abi.getValue() + "-" + getVersion());

        asynchronousTaskService = new AsynchronousTaskService();
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
     * <p>Sets async thread pool parameters.
     *
     * <p>If async thread pool is already initialized existing thread pool is shutdown without
     * interrupting submitted operations.
     *
     * @param coreThreadPoolSize initial thread pool size. See {@link java.util.concurrent.ThreadPoolExecutor#setCorePoolSize(int)}
     * @param maximumThreadPoolSize max thread pool size. See {@link java.util.concurrent.ThreadPoolExecutor#setMaximumPoolSize(int)}
     * @param keepAliveTimeInSeconds thread pool keep alive time. See {@link java.util.concurrent.ThreadPoolExecutor#setKeepAliveTime(long, TimeUnit)}
     */
    public static void setAsyncThreadPool(final int coreThreadPoolSize, final int maximumThreadPoolSize, final int keepAliveTimeInSeconds) {
        asynchronousTaskService.init(coreThreadPoolSize, maximumThreadPoolSize, keepAliveTimeInSeconds);
        asynchronousTaskService.initializeExecutorService();
    }

    /**
     * <p>Synchronously executes FFmpeg with arguments provided.
     *
     * @param arguments FFmpeg command options/arguments
     * @return zero on successful execution, non-zero on error
     */
    public native static int execute(final String ... arguments);

    /**
     * <p>Starts a new asynchronous FFmpeg operation with arguments provided.
     *
     * @param callbackFunction callback function to receive result of this execution
     * @param arguments FFmpeg command options/arguments
     * @return <code>Future</code> instance of asynchronous operation started
     */
    public static Future executeAsync(final Function<Integer, Void> callbackFunction, final String ... arguments) {
        return asynchronousTaskService.runAsynchronously(new Callable<Integer>() {

            @Override
            public Integer call() {
                int returnCode = execute(arguments);
                if (callbackFunction != null) {
                    callbackFunction.apply(returnCode);
                }

                return returnCode;
            }
        });
    }

    /**
     * <p>Stops async thread pool.
     */
    public static void shutdown() {
        asynchronousTaskService.shutdown();
        Log.disableCollectingStdOutErr();
    }

}
