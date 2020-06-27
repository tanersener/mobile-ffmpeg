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
 * - SDLControllerHandler renamed as GenericControllerHandler
 */

package com.arthenica.mobileffmpeg.player;

import android.content.Context;
import android.view.InputDevice;
import android.view.MotionEvent;

public class GenericControllerHandler implements ControllerHandler {
    protected GenericJoystickHandler genericJoystickHandler;
    protected GenericHapticHandler genericHapticHandler;

    public void initialize(final Context context) {
        genericJoystickHandler = new GenericJoystickHandler(this);
        genericHapticHandler = new GenericHapticHandler(context);
    }

    /**
     * Joystick glue code, just a series of stubs that redirect to the JoystickHandler instance.
     */
    public boolean handleJoystickMotionEvent(final MotionEvent event) {
        return genericJoystickHandler.handleMotionEvent(event);
    }

    public void pollInputDevices() {
        genericJoystickHandler.pollInputDevices();
    }

    public void pollHapticDevices() {
        genericHapticHandler.pollHapticDevices();
    }

    public void hapticRun(final int deviceId, final int length) {
        genericHapticHandler.run(deviceId, length);
    }

    /**
     * Check if a given device is considered a possible SDL joystick.
     *
     * @param deviceId device identifier
     * @return true if device is a joystick, false otherwise
     */
    public boolean isDeviceSDLJoystick(final int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if ((device == null) || device.isVirtual() || (deviceId < 0)) {
            return false;
        }

        int sources = device.getSources();
        return (((sources & InputDevice.SOURCE_CLASS_JOYSTICK) == InputDevice.SOURCE_CLASS_JOYSTICK) ||
                ((sources & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD) ||
                ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
        );
    }

}

