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
 * - SDLSurface renamed as PlayerSurface
 */

package com.arthenica.mobileffmpeg.player;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import com.arthenica.mobileffmpeg.FFplay;
import com.arthenica.mobileffmpeg.player.PlayerSession.NativeState;

import java.util.Arrays;

import static android.content.Context.UI_MODE_SERVICE;
import static android.content.res.Configuration.UI_MODE_TYPE_TELEVISION;
import static com.arthenica.mobileffmpeg.Config.TAG;
import static com.arthenica.mobileffmpeg.player.GenericCommandHandler.COMMAND_CHANGE_TITLE;
import static com.arthenica.mobileffmpeg.player.GenericCommandHandler.COMMAND_CHANGE_WINDOW_STYLE;

/**
 * PlayerSurface. This is what we draw on, so we need to know when it's created in order to do
 * anything useful.
 * <p>
 * Because of this, that's where we set up the SDL thread
 */
public class PlayerSurface extends SurfaceView implements SurfaceHolder.Callback,
        View.OnKeyListener, View.OnTouchListener, SensorEventListener, PlayerManager {

    protected Activity activity;
    protected Handler commandHandler;
    protected PlayerClipboard playerClipboard;
    protected SensorManager sensorManager;
    protected Display display;
    protected float width, height;
    protected PlayerSession playerSession;
    protected boolean ready;
    protected boolean resumedCalled;
    protected boolean hasFocus;

    public PlayerSurface(final Context context) {
        super(context);

        onCreated(context);
    }

    public PlayerSurface(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        onCreated(context);
    }

    public PlayerSurface(final Context context, final AttributeSet attrs, final int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        onCreated(context);
    }

    public PlayerSurface(final Context context, final AttributeSet attrs, final int defStyleAttr, final int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);

        onCreated(context);
    }

    protected void onCreated(final Context context) {
        Log.v(TAG, String.format("PlayerSurface created on device: %s and model: %s.", android.os.Build.DEVICE, android.os.Build.MODEL));

        getHolder().addCallback(this);

        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this);
        setOnTouchListener(this);

        commandHandler = new GenericCommandHandler(context);
        playerClipboard = new PlayerClipboard(context);
        display = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);

        // Some arbitrary defaults to avoid a potential division by zero
        width = 1.0f;
        height = 1.0f;

        ready = false;
    }

    public void init(final Activity activity, final View.OnGenericMotionListener motionListener, final PlayerSession playerSession) {
        this.activity = activity;
        setOnGenericMotionListener(motionListener);

        FFplay.setPlayerManager(this);

        this.playerSession = playerSession;
    }

    public void handlePause() {
        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    public void handleResume() {
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this);
        setOnTouchListener(this);
        enableSensor(Sensor.TYPE_ACCELEROMETER, true);
    }

    public Surface getNativeSurface() {
        return getHolder().getSurface();
    }

    @Override
    public void surfaceCreated(final SurfaceHolder ignored) {
    }

    @Override
    public void surfaceDestroyed(final SurfaceHolder holder) {

        // Transition to pause, if needed
        setNextNativeState(NativeState.PAUSED);
        handleNativeState();

        ready = false;
        FFplay.playerOnSurfaceDestroyed();
    }

    @Override
    public void surfaceChanged(final SurfaceHolder holder, final int format, final int width, final int height) {
        int sdlFormat = 0x15151002; // SDL_PIXELFORMAT_RGB565 by default
        switch (format) {
            case PixelFormat.A_8:
                Log.v(TAG, "PlayerSurface using pixel format A_8");
                break;
            case PixelFormat.LA_88:
                Log.v(TAG, "PlayerSurface using pixel format LA_88");
                break;
            case PixelFormat.L_8:
                Log.v(TAG, "PlayerSurface using pixel format L_8");
                break;
            case PixelFormat.RGBA_4444:
                Log.v(TAG, "PlayerSurface using pixel format RGBA_4444");
                sdlFormat = 0x15421002; // SDL_PIXELFORMAT_RGBA4444
                break;
            case PixelFormat.RGBA_5551:
                Log.v(TAG, "PlayerSurface using pixel format RGBA_5551");
                sdlFormat = 0x15441002; // SDL_PIXELFORMAT_RGBA5551
                break;
            case PixelFormat.RGBA_8888:
                Log.v(TAG, "PlayerSurface using pixel format RGBA_8888");
                sdlFormat = 0x16462004; // SDL_PIXELFORMAT_RGBA8888
                break;
            case PixelFormat.RGBX_8888:
                Log.v(TAG, "PlayerSurface using pixel format RGBX_8888");
                sdlFormat = 0x16261804; // SDL_PIXELFORMAT_RGBX8888
                break;
            case PixelFormat.RGB_332:
                Log.v(TAG, "PlayerSurface using pixel format RGB_332");
                sdlFormat = 0x14110801; // SDL_PIXELFORMAT_RGB332
                break;
            case PixelFormat.RGB_565:
                Log.v(TAG, "PlayerSurface using pixel format RGB_565");
                sdlFormat = 0x15151002; // SDL_PIXELFORMAT_RGB565
                break;
            case PixelFormat.RGB_888:
                Log.v(TAG, "PlayerSurface using pixel format RGB_888");
                // Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
                sdlFormat = 0x16161804; // SDL_PIXELFORMAT_RGB888
                break;
            default:
                Log.v(TAG, String.format("PlayerSurface using pixel format unknown %d", format));
                break;
        }

        this.width = width;
        this.height = height;

        FFplay.playerOnResize(width, height, sdlFormat, display.getRefreshRate());

        Log.v(TAG, String.format("PlayerSurface window size: %dx%d.", width, height));

        boolean skip = false;
        int requestedOrientation = getRequestedOrientation();

        if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED) {
            // Accept any
        } else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT || requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT) {
            if (this.width > this.height) {
                skip = true;
            }
        } else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE || requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE) {
            if (this.width < this.height) {
                skip = true;
            }
        }

        // Special Patch for Square Resolution: Black Berry Passport
        if (skip) {
            double min = Math.min(this.width, this.height);
            double max = Math.max(this.width, this.height);

            if (max / min < 1.20) {
                Log.v(TAG, "PlayerSurface don't skip on such aspect-ratio. Could be a square resolution.");
                skip = false;
            }
        }

        if (skip) {
            Log.v(TAG, "PlayerSurface skip .. Surface is not ready.");
            ready = false;
            return;
        }

        /* Surface is ready */
        ready = true;

        /* If the surface has been previously destroyed by onNativeSurfaceDestroyed, recreate it here */
        FFplay.playerOnSurfaceChanged();

        handleNativeState();
    }

    @Override
    public boolean onKey(final View v, final int keyCode, final KeyEvent event) {
        final ControllerHandler controllerHandler = FFplay.getControllerHandler();
        // Dispatch the different events depending on where they come from
        // Some SOURCE_JOYSTICK, SOURCE_DPAD or SOURCE_GAMEPAD are also SOURCE_KEYBOARD
        // So, we try to process them as JOYSTICK/DPAD/GAMEPAD events first, if that fails we try them as KEYBOARD
        //
        // Furthermore, it's possible a game controller has SOURCE_KEYBOARD and
        // SOURCE_JOYSTICK, while its key events arrive from the keyboard source
        // So, retrieve the device itself and check all of its sources
        if (controllerHandler != null && controllerHandler.isDeviceSDLJoystick(event.getDeviceId())) {
            // Note that we process events with specific key codes here
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (FFplay.controllerOnPadDown(event.getDeviceId(), keyCode) == 0) {
                    return true;
                }
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                if (FFplay.controllerOnPadUp(event.getDeviceId(), keyCode) == 0) {
                    return true;
                }
            }
        }

        if ((event.getSource() & InputDevice.SOURCE_KEYBOARD) != 0) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                if (isTextInputEvent(event)) {
                    FFplay.inputCommitText(String.valueOf((char) event.getUnicodeChar()), 1);
                }
                FFplay.playerOnKeyDown(keyCode);
                return true;
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                FFplay.playerOnKeyUp(keyCode);
                return true;
            }
        }

        if ((event.getSource() & InputDevice.SOURCE_MOUSE) != 0) {
            // on some devices key events are sent for mouse BUTTON_BACK/FORWARD presses
            // they are ignored here because sending them as mouse input to SDL is messy
            if ((keyCode == KeyEvent.KEYCODE_BACK) || (keyCode == KeyEvent.KEYCODE_FORWARD)) {
                switch (event.getAction()) {
                    case KeyEvent.ACTION_DOWN:
                    case KeyEvent.ACTION_UP:
                        // mark the event as handled or it will be handled by system
                        // handling KEYCODE_BACK by system will call onBackPressed()
                        return true;
                }
            }
        }

        return false;
    }

    @Override
    public boolean onTouch(final View v, final MotionEvent event) {
        final int touchDevId = event.getDeviceId();
        final int pointerCount = event.getPointerCount();
        int action = event.getActionMasked();
        int pointerFingerId;
        int mouseButton;
        int i = -1;
        float x, y, p;

        if (event.getSource() == InputDevice.SOURCE_MOUSE && FFplay.isSeparateMouseAndTouch()) {
            try {
                mouseButton = (Integer) event.getClass().getMethod("getButtonState").invoke(event);
            } catch (Exception e) {
                mouseButton = 1;    // oh well.
            }
            FFplay.playerOnMouse(mouseButton, action, event.getX(0), event.getY(0));
        } else {
            switch (action) {
                case MotionEvent.ACTION_MOVE:
                    for (i = 0; i < pointerCount; i++) {
                        pointerFingerId = event.getPointerId(i);
                        x = event.getX(i) / width;
                        y = event.getY(i) / height;
                        p = event.getPressure(i);
                        if (p > 1.0f) {
                            // may be larger than 1.0f on some devices
                            // see the documentation of getPressure(i)
                            p = 1.0f;
                        }
                        FFplay.playerOnTouch(touchDevId, pointerFingerId, action, x, y, p);
                    }
                    break;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_DOWN:
                    // Primary pointer up/down, the index is always zero
                    i = 0;
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_POINTER_DOWN:
                    // Non primary pointer up/down
                    if (i == -1) {
                        i = event.getActionIndex();
                    }

                    pointerFingerId = event.getPointerId(i);
                    x = event.getX(i) / width;
                    y = event.getY(i) / height;
                    p = event.getPressure(i);
                    if (p > 1.0f) {
                        // may be larger than 1.0f on some devices
                        // see the documentation of getPressure(i)
                        p = 1.0f;
                    }
                    FFplay.playerOnTouch(touchDevId, pointerFingerId, action, x, y, p);
                    break;

                case MotionEvent.ACTION_CANCEL:
                    for (i = 0; i < pointerCount; i++) {
                        pointerFingerId = event.getPointerId(i);
                        x = event.getX(i) / width;
                        y = event.getY(i) / height;
                        p = event.getPressure(i);
                        if (p > 1.0f) {
                            // may be larger than 1.0f on some devices
                            // see the documentation of getPressure(i)
                            p = 1.0f;
                        }
                        FFplay.playerOnTouch(touchDevId, pointerFingerId, MotionEvent.ACTION_UP, x, y, p);
                    }
                    break;

                default:
                    break;
            }
        }

        return true;
    }

    @Override
    public void onAccuracyChanged(final Sensor sensor, final int accuracy) {
        // TODO
    }

    @Override
    public void onSensorChanged(final SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            float x, y;
            switch (display.getRotation()) {
                case Surface.ROTATION_90:
                    x = -event.values[1];
                    y = event.values[0];
                    break;
                case Surface.ROTATION_270:
                    x = event.values[1];
                    y = -event.values[0];
                    break;
                case Surface.ROTATION_180:
                    x = -event.values[1];
                    y = -event.values[0];
                    break;
                default:
                    x = event.values[0];
                    y = event.values[1];
                    break;
            }
            FFplay.playerOnAccel(-x / SensorManager.GRAVITY_EARTH,
                    y / SensorManager.GRAVITY_EARTH,
                    event.values[2] / SensorManager.GRAVITY_EARTH);
        }
    }

    public void enableSensor(final int sensorType, final boolean enabled) {
        // TODO: This uses getDefaultSensor - what if we have >1 accels?
        if (enabled) {
            sensorManager.registerListener(this,
                    sensorManager.getDefaultSensor(sensorType),
                    SensorManager.SENSOR_DELAY_GAME, null);
        } else {
            sensorManager.unregisterListener(this, sensorManager.getDefaultSensor(sensorType));
        }
    }

    int getRequestedOrientation() {
        final PlayerSession playerSession = this.playerSession;
        if (playerSession == null) {
            return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        } else {
            return playerSession.getRequestedOrientation();
        }
    }

    public void setNextNativeState(final NativeState nextNativeState) {
        final PlayerSession playerSession = this.playerSession;
        if (playerSession != null) {
            playerSession.setNextNativeState(nextNativeState);
        }
    }

    public void setCurrentNativeState(final NativeState currentNativeState) {
        final PlayerSession playerSession = this.playerSession;
        if (playerSession != null) {
            playerSession.setCurrentNativeState(currentNativeState);
        }
    }

    public static boolean isTextInputEvent(final KeyEvent event) {
        if (event.isCtrlPressed()) {
            return false;
        }

        return event.isPrintingKey() || event.getKeyCode() == KeyEvent.KEYCODE_SPACE;
    }

    public void handleNativeState() {
        Log.v(TAG, String.format("PlayerSurface handling nativeState with ready: %s, hasFocus: %s, resumed: %s.", ready, hasFocus, resumedCalled));

        NativeState currentNativeState = null;
        NativeState nextNativeState = null;
        if (playerSession != null) {
            currentNativeState = playerSession.getCurrentNativeState();
            nextNativeState = playerSession.getNextNativeState();
        }

        Log.v(TAG, String.format("PlayerSurface handling nativeState with current:%s, next: %s.", currentNativeState, nextNativeState));

        if (nextNativeState == currentNativeState) {
            // Already in same state, discard.
            return;
        }

        // Try a transition to init state
        if (nextNativeState == NativeState.INIT) {

            setCurrentNativeState(nextNativeState);
            return;
        }

        // Try a transition to paused state
        if (nextNativeState == NativeState.PAUSED) {
            FFplay.playerNativePause();
            handlePause();
            setCurrentNativeState(nextNativeState);
            return;
        }

        // Try a transition to resumed state
        if (nextNativeState == NativeState.RESUMED) {
            play();
        }
    }

    public void play() {
        if (ready && resumedCalled) {
            initialize();
            enableSensor(Sensor.TYPE_ACCELEROMETER, true);
            if (playerSession != null) {
                playerSession.execute();
            }
            FFplay.playerNativeResume();
            handleResume();
            setCurrentNativeState(NativeState.RESUMED);
        } else {
            Log.v(TAG, String.format("PlayerSurface play failed for ready:%s, hasFocus: %s, resumed: %s.", ready, hasFocus, resumedCalled));
        }
    }

    public void setHasFocus(final boolean hasFocus) {
        this.hasFocus = hasFocus;
    }

    public void setResumedCalled(final boolean resumedCalled) {
        this.resumedCalled = resumedCalled;
    }

    public boolean sendCommand(final int command, final Object data) {
        Message message = commandHandler.obtainMessage();
        message.arg1 = command;
        message.obj = data;
        return commandHandler.sendMessage(message);
    }

    public void initialize() {
        setHasFocus(true);
        setNextNativeState(NativeState.INIT);
        setCurrentNativeState(NativeState.INIT);
    }

    public boolean setActivityTitle(final String title) {
        return sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    public void setWindowStyle(final boolean fullScreen) {
        sendCommand(COMMAND_CHANGE_WINDOW_STYLE, fullScreen ? 1 : 0);
    }

    public void setOrientation(final int w, final int h, final boolean resizable, final String hint) {
        int orientation = -1;

        if (hint.contains("LandscapeRight") && hint.contains("LandscapeLeft")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        } else if (hint.contains("LandscapeRight")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        } else if (hint.contains("LandscapeLeft")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
        } else if (hint.contains("Portrait") && hint.contains("PortraitUpsideDown")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
        } else if (hint.contains("Portrait")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        } else if (hint.contains("PortraitUpsideDown")) {
            orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
        }

        /* no valid hint */
        if (orientation == -1) {
            if (resizable) {
                /* no fixed orientation */
            } else {
                if (w > h) {
                    orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
                } else {
                    orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
                }
            }
        }

        Log.v(TAG, String.format("PlayerSurface set orientation:%d, width:%d, height:%d, resizable:%s and hint:%s.", orientation, w, h, resizable, hint));

        if (orientation != -1) {
            activity.setRequestedOrientation(orientation);
        }
    }

    public boolean isScreenKeyboardShown() {
        return false;
    }

    public boolean sendMessage(final int command, final int param) {
        return sendCommand(command, param);
    }

    public boolean isAndroidTV() {
        UiModeManager uiModeManager = (UiModeManager) getContext().getSystemService(UI_MODE_SERVICE);
        return (uiModeManager.getCurrentModeType() == UI_MODE_TYPE_TELEVISION);
    }

    public DisplayMetrics getDisplayDPI() {
        return getContext().getResources().getDisplayMetrics();
    }

    public boolean getManifestEnvironmentVariables() {
        try {
            ApplicationInfo applicationInfo = getContext().getPackageManager().getApplicationInfo(getContext().getPackageName(), PackageManager.GET_META_DATA);
            Bundle bundle = applicationInfo.metaData;
            if (bundle == null) {
                return false;
            }
            String prefix = "SDL_ENV.";
            final int trimLength = prefix.length();
            for (String key : bundle.keySet()) {
                if (key.startsWith(prefix)) {
                    String name = key.substring(trimLength);
                    String value = bundle.get(key).toString();
                    FFplay.playerNativeSetenv(name, value);
                }
            }

            return true;
        } catch (final Exception e) {
            Log.i(TAG, "PlayerSurface failed to set environment variables. " + e.toString());
        }
        return false;
    }

    public boolean showTextInput(final int x, final int y, final int w, final int h) {
        return false;
    }

    public int[] inputGetInputDeviceIds(final int sources) {
        int[] ids = InputDevice.getDeviceIds();
        int[] filtered = new int[ids.length];
        int used = 0;

        for (int id : ids) {
            InputDevice device = InputDevice.getDevice(id);
            if ((device != null) && ((device.getSources() & sources) != 0)) {
                filtered[used++] = device.getId();
            }
        }

        return Arrays.copyOf(filtered, used);
    }

    public int showMessageBox(
            final int flags,
            final String title,
            final String message,
            final int[] buttonFlags,
            final int[] buttonIds,
            final String[] buttonTexts,
            final int[] colors) {

        Log.i(TAG, String.format("PlayerSurface was asked to showMessageBox for title: %s, message: %s and %d buttons: %s.", title, message, (buttonIds != null) ? buttonIds.length : 0, Arrays.toString(buttonTexts)));

        return -1;
    }

    public boolean clipboardHasText() {
        return playerClipboard.clipboardHasText();
    }

    public String clipboardGetText() {
        return playerClipboard.clipboardGetText();
    }

    public void clipboardSetText(final String string) {
        playerClipboard.clipboardSetText(string);
    }

}
