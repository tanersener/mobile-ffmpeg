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

package com.arthenica.mobileffmpeg.util;

import android.util.Log;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class AsynchronousTaskService {

    protected static final String TAG = "mobile-ffmpeg-android";

    protected final BlockingQueue<Runnable> queue;

    protected final Object executorServiceLock;

    protected ThreadPoolExecutor executorService;

    protected int coreThreadPoolSize;

    protected int maximumThreadPoolSize;

    protected int keepAliveTimeInSeconds;

    public AsynchronousTaskService() {
        this(1, 5, 30);
    }

    public AsynchronousTaskService(final int coreThreadPoolSize, final int maximumThreadPoolSize, final int keepAliveTimeInSeconds) {
        this.executorServiceLock = new Object();
        this.queue = new LinkedBlockingQueue<>();
        this.executorService = null;

        this.coreThreadPoolSize = coreThreadPoolSize;
        this.maximumThreadPoolSize = maximumThreadPoolSize;
        this.keepAliveTimeInSeconds = keepAliveTimeInSeconds;
    }

    public void init(final int coreThreadPoolSize, final int maximumThreadPoolSize, final int keepAliveTimeInSeconds) {
        this.coreThreadPoolSize = coreThreadPoolSize;
        this.maximumThreadPoolSize = maximumThreadPoolSize;
        this.keepAliveTimeInSeconds = keepAliveTimeInSeconds;
    }

    public void initializeExecutorService() {
        synchronized (executorServiceLock) {
            if (executorService != null) {
                executorService.shutdown();
            }

            executorService = new ThreadPoolExecutor(coreThreadPoolSize, maximumThreadPoolSize, keepAliveTimeInSeconds, TimeUnit.SECONDS, queue, new AsyncThreadFactory());
        }
    }

    public <T> Future<T> runAsynchronously(final Callable<T> callable) {

        /* INITIALIZE EXECUTOR SERVICE ONLY IF NECESSARY */
        if (executorService == null) {
            initializeExecutorService();
        }

        final String className = callable.getClass().getSimpleName();

        try {
            return executorService.submit(callable);
        } catch (Exception e) {
            Log.w(TAG, String.format("Failed to run asynchronous task %s. Running it synchronously.", className), e);
            return runSynchronously(callable);
        }
    }

    public <T> Future<T> runSynchronously(final Callable<T> callable) {
        T called = null;

        /* INITIALIZE EXECUTOR SERVICE ONLY IF NECESSARY */
        if (executorService == null) {
            initializeExecutorService();
        }

        final String className = callable.getClass().getSimpleName();

        try {
            called = callable.call();
            return new SynchronousResultFuture<>(called, true);
        } catch (final Exception e) {
            Log.d(TAG, "Failed to run asynchronous task " + className + " synchronously.", e);
            return new SynchronousResultFuture<>(called, true);
        }
    }

    public void shutdown() {
        if (executorService != null) {
            synchronized (executorServiceLock) {
                executorService.shutdown();
            }
        }
    }

}
