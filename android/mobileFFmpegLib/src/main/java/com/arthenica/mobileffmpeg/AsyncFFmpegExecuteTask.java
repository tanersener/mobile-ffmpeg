/*
 * Copyright (c) 2018-2020 Taner Sener
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

import android.os.AsyncTask;

/**
 * <p>Utility class to execute an FFmpeg command asynchronously.
 *
 * @author Taner Sener
 */
public class AsyncFFmpegExecuteTask extends AsyncTask<Void, Integer, Integer> {
    private final String[] arguments;
    private final ExecuteCallback executeCallback;
    private final Long executionId;

    public AsyncFFmpegExecuteTask(final String command, final ExecuteCallback executeCallback) {
        this(FFmpeg.parseArguments(command), executeCallback);
    }

    public AsyncFFmpegExecuteTask(final String[] arguments, final ExecuteCallback executeCallback) {
        this(FFmpeg.DEFAULT_EXECUTION_ID, arguments, executeCallback);
    }

    public AsyncFFmpegExecuteTask(final long executionId, final String command, final ExecuteCallback executeCallback) {
        this(executionId, FFmpeg.parseArguments(command), executeCallback);
    }

    public AsyncFFmpegExecuteTask(final long executionId, final String[] arguments, final ExecuteCallback executeCallback) {
        this.executionId = executionId;
        this.arguments = arguments;
        this.executeCallback = executeCallback;
    }

    @Override
    protected Integer doInBackground(final Void... unused) {
        return Config.ffmpegExecute(executionId, this.arguments);
    }

    @Override
    protected void onPostExecute(final Integer rc) {
        if (executeCallback != null) {
            executeCallback.apply(executionId, rc);
        }
    }

}
