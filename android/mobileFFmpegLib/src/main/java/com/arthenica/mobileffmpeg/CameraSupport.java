/*
 * Copyright (c) 2019 Taner Sener
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
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg;

import android.content.Context;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.os.Build;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

import static android.content.Context.CAMERA_SERVICE;
import static com.arthenica.mobileffmpeg.Config.TAG;

/**
 * Utility class for camera devices.
 *
 * @author Taner Sener
 */
class CameraSupport {

    /**
     * <p>Compatibility method for extracting supported camera ids.
     *
     * @param context application context
     * @return returns the list of supported camera ids
     */
    static List<String> extractSupportedCameraIds(final Context context) {
        final List<String> detectedCameraIdList = new ArrayList<>();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            try {
                final CameraManager manager = (CameraManager) context.getSystemService(CAMERA_SERVICE);
                if (manager != null) {
                    final String[] cameraIdList = manager.getCameraIdList();

                    for (String cameraId : cameraIdList) {
                        final CameraCharacteristics chars = manager.getCameraCharacteristics(cameraId);
                        final Integer cameraSupport = chars.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);

                        if (cameraSupport != null && cameraSupport == CameraMetadata.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
                            Log.d(TAG, "Detected camera with id " + cameraId + " has LEGACY hardware level which is not supported by Android Camera2 NDK API.");
                        } else if (cameraSupport != null) {
                            detectedCameraIdList.add(cameraId);
                        }
                    }
                }
            } catch (final CameraAccessException e) {
                Log.w(TAG, "Detecting camera ids failed.", e);
            }
        }

        return detectedCameraIdList;
    }

}
