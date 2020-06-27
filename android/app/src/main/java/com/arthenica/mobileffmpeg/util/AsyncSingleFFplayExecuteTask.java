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
import android.util.Log;

import com.arthenica.mobileffmpeg.FFplay;

import static com.arthenica.mobileffmpeg.Config.TAG;

public class AsyncSingleFFplayExecuteTask extends AsyncTask<String, Integer, Integer> {
    private final String command;

    public AsyncSingleFFplayExecuteTask(final String command) {
        this.command = command;
    }

    @Override
    protected Integer doInBackground(final String... ignored) {

        Log.v(TAG, String.format("Running FFplay for %s.", command));

        int rc = FFplay.execute(command.split(" "));

        Log.v(TAG, String.format("Finished running FFplay for %s.", command));

        return rc;
    }

    @Override
    protected void onPostExecute(final Integer ignored) {
    }

}
