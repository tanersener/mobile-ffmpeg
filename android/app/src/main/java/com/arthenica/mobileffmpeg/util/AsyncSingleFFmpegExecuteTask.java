/*
 * Copyright (c) 2018-2019 Taner Sener
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

import android.os.AsyncTask;

import com.arthenica.mobileffmpeg.Config;
import com.arthenica.mobileffmpeg.FFmpeg;

public class AsyncSingleFFmpegExecuteTask extends AsyncTask<String, Integer, Integer> {
    private final String command;
    private final SingleExecuteCallback singleExecuteCallback;

    public AsyncSingleFFmpegExecuteTask(final String command, final SingleExecuteCallback singleExecuteCallback) {
        this.command = command;
        this.singleExecuteCallback = singleExecuteCallback;
    }

    @Override
    protected Integer doInBackground(final String... arguments) {
        return FFmpeg.execute(command);
    }

    @Override
    protected void onPostExecute(final Integer rc) {
        if (singleExecuteCallback != null) {
            singleExecuteCallback.apply(rc, Config.getLastCommandOutput());
        }
    }

}
