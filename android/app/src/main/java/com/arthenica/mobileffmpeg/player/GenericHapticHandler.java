/*
 * Simple DirectMedia Layer
 * Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * CHANGES 07.2020
 * - SDLHapticHandler renamed as GenericHapticHandler
 * - SDLHaptic class renamed as Haptic
 * - Haptic class refactored
 */

package com.arthenica.mobileffmpeg.player;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Vibrator;
import android.view.InputDevice;

import com.arthenica.mobileffmpeg.FFplay;

import java.util.ArrayList;

/**
 * <p>Generic haptic handler for FFplay Controller.
 */
public class GenericHapticHandler {

    public static class Haptic {
        public final int deviceId;
        public final String name;
        public final Vibrator vibrator;

        public Haptic(final int deviceId, final String name, final Vibrator vibrator) {
            this.deviceId = deviceId;
            this.name = name;
            this.vibrator = vibrator;
        }

    }

    protected final ArrayList<Haptic> hapticList;
    protected final Vibrator vibratorService;

    public GenericHapticHandler(final Context context) {
        hapticList = new ArrayList<>();
        vibratorService = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
    }

    @SuppressLint("MissingPermission")
    public void run(final int deviceId, final int length) {
        Haptic haptic = getHaptic(deviceId);
        if (haptic != null) {
            haptic.vibrator.vibrate(length);
        }
    }

    public void pollHapticDevices() {

        final int DEVICE_ID_VIBRATOR_SERVICE = 999999;
        boolean hasVibratorService = false;

        final int[] deviceIdArray = InputDevice.getDeviceIds();
        // It helps processing the device ids in reverse order
        // For example, in the case of the XBox 360 wireless dongle,
        // so the first controller seen by SDL matches what the receiver
        // considers to be the first controller
        for (int i = deviceIdArray.length - 1; i > -1; i--) {
            Haptic haptic = getHaptic(deviceIdArray[i]);
            if (haptic == null) {
                InputDevice device = InputDevice.getDevice(deviceIdArray[i]);
                Vibrator vib = device.getVibrator();
                if (vib.hasVibrator()) {
                    haptic = new Haptic(deviceIdArray[i], device.getName(), vib);
                    hapticList.add(haptic);
                    FFplay.controllerAddHaptic(haptic.deviceId, haptic.name);
                }
            }
        }

        /* Check VIBRATOR_SERVICE */
        if (vibratorService != null) {
            hasVibratorService = vibratorService.hasVibrator();

            if (hasVibratorService) {
                Haptic haptic = getHaptic(DEVICE_ID_VIBRATOR_SERVICE);
                if (haptic == null) {
                    haptic = new Haptic(DEVICE_ID_VIBRATOR_SERVICE, "VIBRATOR_SERVICE", vibratorService);
                    hapticList.add(haptic);
                    FFplay.controllerAddHaptic(haptic.deviceId, haptic.name);
                }
            }
        }

        /* Check removed devices */
        ArrayList<Integer> removedDevices = new ArrayList<>();
        for (int i = 0; i < hapticList.size(); i++) {
            int deviceId = hapticList.get(i).deviceId;
            int j;
            for (j = 0; j < deviceIdArray.length; j++) {
                if (deviceId == deviceIdArray[j]) break;
            }

            if (deviceId == DEVICE_ID_VIBRATOR_SERVICE && hasVibratorService) {
                // don't remove the vibrator if it is still present
            } else if (j == deviceIdArray.length) {
                removedDevices.add(deviceId);
            }
        }

        for (int i = 0; i < removedDevices.size(); i++) {
            int deviceId = removedDevices.get(i);
            FFplay.controllerRemoveHaptic(deviceId);
            for (int j = 0; j < hapticList.size(); j++) {
                if (hapticList.get(j).deviceId == deviceId) {
                    hapticList.remove(j);
                    break;
                }
            }
        }
    }

    protected Haptic getHaptic(final int deviceId) {
        for (int i = 0; i < hapticList.size(); i++) {
            if (hapticList.get(i).deviceId == deviceId) {
                return hapticList.get(i);
            }
        }
        return null;
    }

}
