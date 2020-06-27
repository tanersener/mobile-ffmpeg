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
 * - SDLJoystickHandler renamed as GenericJoystickHandler
 * - SDLJoystick class renamed as Joystick
 * - Joystick class refactored
 */

package com.arthenica.mobileffmpeg.player;

import android.view.InputDevice;
import android.view.MotionEvent;

import com.arthenica.mobileffmpeg.FFplay;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * <p>Generic joystick handler for FFplay Controller.
 */
public class GenericJoystickHandler {

    public static class Joystick {
        public final int deviceId;
        public final String name;
        public final String desc;
        public final ArrayList<InputDevice.MotionRange> axes;
        public final ArrayList<InputDevice.MotionRange> hats;

        public Joystick(final int deviceId, final String name, final String desc, final ArrayList<InputDevice.MotionRange> axes, final ArrayList<InputDevice.MotionRange> hats) {
            this.deviceId = deviceId;
            this.name = name;
            this.desc = desc;
            this.axes = axes;
            this.hats = hats;
        }
    }

    public static class RangeComparator implements Comparator<InputDevice.MotionRange> {
        @Override
        public int compare(final InputDevice.MotionRange arg0, final InputDevice.MotionRange arg1) {
            return arg0.getAxis() - arg1.getAxis();
        }
    }

    protected final ArrayList<Joystick> joystickList;
    protected final ControllerHandler controllerHandler;

    public GenericJoystickHandler(final ControllerHandler controllerHandler) {
        this.joystickList = new ArrayList<>();
        this.controllerHandler = controllerHandler;
    }

    public void pollInputDevices() {
        int[] deviceIds = InputDevice.getDeviceIds();
        // It helps processing the device ids in reverse order
        // For example, in the case of the XBox 360 wireless dongle,
        // so the first controller seen by SDL matches what the receiver
        // considers to be the first controller

        for (int i = deviceIds.length - 1; i > -1; i--) {
            Joystick joystick = getJoystick(deviceIds[i]);
            if (joystick == null) {
                InputDevice joystickDevice = InputDevice.getDevice(deviceIds[i]);
                if (controllerHandler.isDeviceSDLJoystick(deviceIds[i])) {
                    joystick = new Joystick(deviceIds[i], joystickDevice.getName(), getJoystickDescriptor(joystickDevice), new ArrayList<InputDevice.MotionRange>(), new ArrayList<InputDevice.MotionRange>());
                    List<InputDevice.MotionRange> ranges = joystickDevice.getMotionRanges();
                    Collections.sort(ranges, new RangeComparator());

                    for (InputDevice.MotionRange range : ranges) {
                        if ((range.getSource() & InputDevice.SOURCE_CLASS_JOYSTICK) != 0) {
                            if (range.getAxis() == MotionEvent.AXIS_HAT_X || range.getAxis() == MotionEvent.AXIS_HAT_Y) {
                                joystick.hats.add(range);
                            } else {
                                joystick.axes.add(range);
                            }
                        }
                    }

                    joystickList.add(joystick);
                    FFplay.controllerAddJoystick(joystick.deviceId, joystick.name, joystick.desc, 0, -1, joystick.axes.size(), joystick.hats.size() / 2, 0);
                }
            }
        }

        /* Check removed devices */
        ArrayList<Integer> removedDevices = new ArrayList<>();
        for (int i = 0; i < joystickList.size(); i++) {
            int deviceId = joystickList.get(i).deviceId;
            int j;
            for (j = 0; j < deviceIds.length; j++) {
                if (deviceId == deviceIds[j]) break;
            }
            if (j == deviceIds.length) {
                removedDevices.add(deviceId);
            }
        }

        for (int i = 0; i < removedDevices.size(); i++) {
            int deviceId = removedDevices.get(i);
            FFplay.controllerRemoveJoystick(deviceId);
            for (int j = 0; j < joystickList.size(); j++) {
                if (joystickList.get(j).deviceId == deviceId) {
                    joystickList.remove(j);
                    break;
                }
            }
        }
    }

    protected Joystick getJoystick(int deviceId) {
        for (int i = 0; i < joystickList.size(); i++) {
            if (joystickList.get(i).deviceId == deviceId) {
                return joystickList.get(i);
            }
        }
        return null;
    }

    public boolean handleMotionEvent(final MotionEvent event) {
        if ((event.getSource() & InputDevice.SOURCE_JOYSTICK) != 0) {
            int actionPointerIndex = event.getActionIndex();
            int action = event.getActionMasked();

            if (action == MotionEvent.ACTION_MOVE) {
                Joystick joystick = getJoystick(event.getDeviceId());
                if (joystick != null) {
                    for (int i = 0; i < joystick.axes.size(); i++) {
                        InputDevice.MotionRange range = joystick.axes.get(i);
                        /* Normalize the value to -1...1 */
                        float value = (event.getAxisValue(range.getAxis(), actionPointerIndex) - range.getMin()) / range.getRange() * 2.0f - 1.0f;
                        FFplay.controllerOnJoy(joystick.deviceId, i, value);
                    }
                    for (int i = 0; i < joystick.hats.size(); i += 2) {
                        int hatX = Math.round(event.getAxisValue(joystick.hats.get(i).getAxis(), actionPointerIndex));
                        int hatY = Math.round(event.getAxisValue(joystick.hats.get(i + 1).getAxis(), actionPointerIndex));
                        FFplay.controllerOnHat(joystick.deviceId, i / 2, hatX, hatY);
                    }
                }
            }
        }

        return true;
    }

    public String getJoystickDescriptor(final InputDevice joystickDevice) {
        String desc = joystickDevice.getDescriptor();

        if (desc != null && !desc.isEmpty()) {
            return desc;
        }

        return joystickDevice.getName();
    }

}
