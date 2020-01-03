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

import com.arthenica.mobileffmpeg.FFmpeg;
import com.arthenica.mobileffmpeg.MediaInformation;

public class AsyncSingleGetMediaInformationTask extends AsyncTask<String, MediaInformation, MediaInformation> {
    private final String path;
    private final SingleGetMediaInformationCallback singleGetMediaInformationCallback;

    public AsyncSingleGetMediaInformationTask(final String path, final SingleGetMediaInformationCallback singleGetMediaInformationCallback) {
        this.path = path;
        this.singleGetMediaInformationCallback = singleGetMediaInformationCallback;
    }

    @Override
    protected MediaInformation doInBackground(final String... arguments) {
        return FFmpeg.getMediaInformation(path);
    }

    @Override
    protected void onPostExecute(final MediaInformation mediaInformation) {
        if (singleGetMediaInformationCallback != null) {
            singleGetMediaInformationCallback.apply(mediaInformation);
        }
    }

}
