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

package com.arthenica.mobileffmpeg.sdl;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.os.Vibrator;
import android.view.InputDevice;

import java.util.ArrayList;

public class SDLHapticHandler {

    private ArrayList<SDLHaptic> mHaptics;

    public SDLHapticHandler() {
        mHaptics = new ArrayList<SDLHaptic>();
    }

    @SuppressLint("MissingPermission")
    public void run(int device_id, int length) {
        SDLHaptic haptic = getHaptic(device_id);
        if (haptic != null) {
            haptic.vib.vibrate(length);
        }
    }

    public void pollHapticDevices() {

        final int deviceId_VIBRATOR_SERVICE = 999999;
        boolean hasVibratorService = false;

        int[] deviceIds = InputDevice.getDeviceIds();
        // It helps processing the device ids in reverse order
        // For example, in the case of the XBox 360 wireless dongle,
        // so the first controller seen by SDL matches what the receiver
        // considers to be the first controller

        if (Build.VERSION.SDK_INT >= 16) {
            for (int i = deviceIds.length - 1; i > -1; i--) {
                SDLHaptic haptic = getHaptic(deviceIds[i]);
                if (haptic == null) {
                    InputDevice device = InputDevice.getDevice(deviceIds[i]);
                    Vibrator vib = device.getVibrator();
                    if (vib.hasVibrator()) {
                        haptic = new SDLHaptic();
                        haptic.device_id = deviceIds[i];
                        haptic.name = device.getName();
                        haptic.vib = vib;
                        mHaptics.add(haptic);
                        SDLControllerManager.nativeAddHaptic(haptic.device_id, haptic.name);
                    }
                }
            }
        }

        /* Check VIBRATOR_SERVICE */
        Vibrator vib = (Vibrator) SDL.getContext().getSystemService(Context.VIBRATOR_SERVICE);
        if (vib != null) {
            hasVibratorService = vib.hasVibrator();

            if (hasVibratorService) {
                SDLHaptic haptic = getHaptic(deviceId_VIBRATOR_SERVICE);
                if (haptic == null) {
                    haptic = new SDLHaptic();
                    haptic.device_id = deviceId_VIBRATOR_SERVICE;
                    haptic.name = "VIBRATOR_SERVICE";
                    haptic.vib = vib;
                    mHaptics.add(haptic);
                    SDLControllerManager.nativeAddHaptic(haptic.device_id, haptic.name);
                }
            }
        }

        /* Check removed devices */
        ArrayList<Integer> removedDevices = new ArrayList<Integer>();
        for (int i = 0; i < mHaptics.size(); i++) {
            int device_id = mHaptics.get(i).device_id;
            int j;
            for (j = 0; j < deviceIds.length; j++) {
                if (device_id == deviceIds[j]) break;
            }

            if (device_id == deviceId_VIBRATOR_SERVICE && hasVibratorService) {
                // don't remove the vibrator if it is still present
            } else if (j == deviceIds.length) {
                removedDevices.add(device_id);
            }
        }

        for (int i = 0; i < removedDevices.size(); i++) {
            int device_id = removedDevices.get(i);
            SDLControllerManager.nativeRemoveHaptic(device_id);
            for (int j = 0; j < mHaptics.size(); j++) {
                if (mHaptics.get(j).device_id == device_id) {
                    mHaptics.remove(j);
                    break;
                }
            }
        }
    }

    protected SDLHaptic getHaptic(int device_id) {
        for (int i = 0; i < mHaptics.size(); i++) {
            if (mHaptics.get(i).device_id == device_id) {
                return mHaptics.get(i);
            }
        }
        return null;
    }

    class SDLHaptic {
        public int device_id;
        public String name;
        public Vibrator vib;
    }
}
