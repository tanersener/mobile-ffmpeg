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

import java.io.IOException;

import static com.arthenica.mobileffmpeg.test.MainActivity.TAG;

public class AsyncCatImageTask extends AsyncTask<String, Integer, Integer> {

    @Override
    protected Integer doInBackground(String... inputs) {
        try {
            final String asyncCommand = "cat " + inputs[0] + " > " + inputs[1];
            Log.d(TAG, String.format("Starting async cat image command: %s", asyncCommand));

            final Process process = Runtime.getRuntime().exec(new String[]{"sh", "-c", asyncCommand});
            int rc = process.waitFor();

            Log.d(TAG, String.format("Async cat image command: %s exited with %d.", asyncCommand, rc));

            return rc;
        } catch (final IOException | InterruptedException e) {
            Log.e(TAG, String.format("Async cat image command failed for %s.", inputs[0]), e);
            return -1;
        }
    }

}
