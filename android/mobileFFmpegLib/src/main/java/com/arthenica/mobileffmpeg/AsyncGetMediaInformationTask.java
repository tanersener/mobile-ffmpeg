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

import com.arthenica.mobileffmpeg.FFprobe;
import com.arthenica.mobileffmpeg.GetMediaInformationCallback;
import com.arthenica.mobileffmpeg.MediaInformation;

/**
 * <p>Utility class to get media information asynchronously.
 *
 * @author Taner Sener
 */
public class AsyncGetMediaInformationTask extends AsyncTask<String, MediaInformation, MediaInformation> {
    private final String path;
    private final GetMediaInformationCallback getMediaInformationCallback;

    public AsyncGetMediaInformationTask(final String path, final GetMediaInformationCallback getMediaInformationCallback) {
        this.path = path;
        this.getMediaInformationCallback = getMediaInformationCallback;
    }

    @Override
    protected MediaInformation doInBackground(final String... arguments) {
        return FFprobe.getMediaInformation(path);
    }

    @Override
    protected void onPostExecute(final MediaInformation mediaInformation) {
        if (getMediaInformationCallback != null) {
            getMediaInformationCallback.apply(mediaInformation);
        }
    }

}
