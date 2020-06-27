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
 * - SDLGenericMotionListener renamed as GenericMotionListener
 */

package com.arthenica.mobileffmpeg.player;

import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

import com.arthenica.mobileffmpeg.FFplay;

/**
 * <p>Generic motion listener for FFplay Player.
 */
public class GenericMotionListener implements View.OnGenericMotionListener {

    @Override
    public boolean onGenericMotion(final View view, final MotionEvent event) {
        ControllerHandler controllerHandler = FFplay.getControllerHandler();
        float x, y;
        int action;

        switch (event.getSource()) {
            case InputDevice.SOURCE_JOYSTICK:
            case InputDevice.SOURCE_GAMEPAD:
            case InputDevice.SOURCE_DPAD: {
                if (controllerHandler != null) {
                    return controllerHandler.handleJoystickMotionEvent(event);
                } else {
                    return false;
                }
            }
            case InputDevice.SOURCE_MOUSE: {
                if (!FFplay.isSeparateMouseAndTouch()) {
                    break;
                }
                action = event.getActionMasked();
                switch (action) {
                    case MotionEvent.ACTION_SCROLL: {
                        x = event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0);
                        y = event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0);

                        FFplay.playerOnMouse(0, action, x, y);
                        return true;
                    }
                    case MotionEvent.ACTION_HOVER_MOVE: {
                        x = event.getX(0);
                        y = event.getY(0);

                        FFplay.playerOnMouse(0, action, x, y);
                        return true;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            default: {
                break;
            }
        }

        return false;
    }

}
