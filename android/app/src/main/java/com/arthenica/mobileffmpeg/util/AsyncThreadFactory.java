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

import android.support.annotation.NonNull;

import java.text.MessageFormat;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

public class AsyncThreadFactory implements ThreadFactory {

    protected static final String DEFAULT_THREAD_NAME_PREFIX = "AsyncTaskThread";

    protected static AtomicInteger threadCounter;

    static {
        threadCounter = new AtomicInteger(1);
    }

    private final String namePrefix;

    public AsyncThreadFactory() {
        this(DEFAULT_THREAD_NAME_PREFIX);
    }

    public AsyncThreadFactory(final String namePrefix) {
        this.namePrefix = namePrefix;
    }

    @Override
    public Thread newThread(@NonNull final Runnable runnable) {
        return new Thread(runnable, MessageFormat.format("{0}-{1,number,#}", namePrefix, threadCounter.getAndIncrement()));
    }

}
