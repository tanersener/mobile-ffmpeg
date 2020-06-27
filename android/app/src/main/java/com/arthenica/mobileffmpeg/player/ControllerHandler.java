/*
 * Copyright (c) 2020 Taner Sener
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

package com.arthenica.mobileffmpeg.player;

import android.content.Context;
import android.view.MotionEvent;

public interface ControllerHandler {

    void initialize(final Context context);

    boolean handleJoystickMotionEvent(final MotionEvent event);

    void pollInputDevices();

    void pollHapticDevices();

    void hapticRun(final int deviceId, final int length);

    boolean isDeviceSDLJoystick(final int deviceId);

}
