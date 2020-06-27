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
 * - SDLActivity renamed as FullScreenActivity
 */

package com.arthenica.mobileffmpeg.player;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

import com.arthenica.mobileffmpeg.FFplay;

import static com.arthenica.mobileffmpeg.Config.TAG;
import static com.arthenica.mobileffmpeg.player.PlayerSession.FFPLAY_COMMAND;
import static com.arthenica.mobileffmpeg.player.PlayerSession.NativeState;

public class FullScreenActivity extends Activity {
    protected PlayerSurface playerSurface;
    protected ViewGroup viewLayout;
    protected PlayerSession playerSession;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        String ffplayCommand = intent.getStringExtra(FFPLAY_COMMAND);

        if (ffplayCommand == null) {
            Log.i(TAG, "FullScreenActivity created with empty ffplay command.");
        } else {
            Log.v(TAG, "FullScreenActivity created.");
        }

        playerSession = new PlayerSession(getRequestedOrientation(), this, ffplayCommand);
        playerSurface = new PlayerSurface(this);
        playerSurface.init(this, new GenericMotionListener(), playerSession);

        viewLayout = new RelativeLayout(this);
        viewLayout.addView(playerSurface);

        setContentView(viewLayout);
        setWindowStyle(false);
    }

    @Override
    protected void onPause() {
        Log.v(TAG, "FullScreenActivity paused.");
        setNextNativeState(NativeState.PAUSED);
        setResumedCalled(false);
        handleNativeState();
        super.onPause();
    }

    @Override
    protected void onResume() {
        Log.v(TAG, "FullScreenActivity resumed.");
        setNextNativeState(NativeState.RESUMED);
        setResumedCalled(true);
        handleNativeState();
        super.onResume();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        Log.v(TAG, String.format("FullScreenActivity window focus changed, hasFocus: %s.", hasFocus));
        setHasFocus(hasFocus);
        if (hasFocus) {
            setNextNativeState(NativeState.RESUMED);
        } else {
            setNextNativeState(NativeState.PAUSED);
        }
        handleNativeState();
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    public void onLowMemory() {
        Log.v(TAG, "FullScreenActivity is on low memory.");
        FFplay.playerNativeLowMemory();
        super.onLowMemory();
    }

    @Override
    protected void onDestroy() {
        Log.v(TAG, "FullScreenActivity destroyed.");
        setNextNativeState(NativeState.PAUSED);
        handleNativeState();

        // Send a quit message to the application
        FFplay.playerNativeQuit();
        super.onDestroy();
    }

    @Override
    public boolean dispatchKeyEvent(final KeyEvent event) {
        int keyCode = event.getKeyCode();

        // Ignore certain special keys so they're handled by Android
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
                keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
                keyCode == KeyEvent.KEYCODE_CAMERA ||
                keyCode == KeyEvent.KEYCODE_ZOOM_IN ||
                keyCode == KeyEvent.KEYCODE_ZOOM_OUT) {
            return false;
        }

        return super.dispatchKeyEvent(event);
    }

    protected void setNextNativeState(final NativeState nextNativeState) {
        PlayerSession playerSession = this.playerSession;
        if (playerSession != null) {
            playerSession.setNextNativeState(nextNativeState);
        }
    }

    protected void handleNativeState() {
        PlayerSurface playerSurface = this.playerSurface;
        if (playerSurface != null) {
            playerSurface.handleNativeState();
        }
    }

    protected void setHasFocus(final boolean hasFocus) {
        PlayerSurface playerSurface = this.playerSurface;
        if (playerSurface != null) {
            playerSurface.setHasFocus(hasFocus);
        }
    }

    protected void setResumedCalled(final boolean resumedCalled) {
        PlayerSurface playerSurface = this.playerSurface;
        if (playerSurface != null) {
            playerSurface.setResumedCalled(resumedCalled);
        }
    }

    protected void setWindowStyle(final boolean fullScreen) {
        PlayerSurface playerSurface = this.playerSurface;
        if (playerSurface != null) {
            playerSurface.setWindowStyle(fullScreen);
        }
    }

    public PlayerSurface getPlayerSurface() {
        return playerSurface;
    }

    public PlayerSession getPlayerSession() {
        return playerSession;
    }

}
