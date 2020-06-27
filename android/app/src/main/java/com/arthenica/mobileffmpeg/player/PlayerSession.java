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
import android.os.AsyncTask;

import com.arthenica.mobileffmpeg.FFplay;
import com.arthenica.mobileffmpeg.util.AsyncSingleFFplayExecuteTask;

public class PlayerSession {

    public static final String FFPLAY_COMMAND = "ffplayCommand";

    public enum NativeState {
        INIT, RESUMED, PAUSED
    }

    protected int requestedOrientation;
    protected Context context;
    protected final String command;
    protected NativeState nextNativeState;
    protected NativeState currentNativeState;

    public PlayerSession(final int requestedOrientation, final Context context, final String command) {
        this.requestedOrientation = requestedOrientation;
        this.context = context;
        this.command = command;
    }

    public NativeState getNextNativeState() {
        return nextNativeState;
    }

    public void setNextNativeState(final NativeState nextNativeState) {
        this.nextNativeState = nextNativeState;
    }

    public NativeState getCurrentNativeState() {
        return currentNativeState;
    }

    public void setCurrentNativeState(final NativeState currentNativeState) {
        this.currentNativeState = currentNativeState;
    }

    public Context getContext() {
        return context;
    }

    public void setContext(final Context context) {
        this.context = context;
    }

    public int getRequestedOrientation() {
        return requestedOrientation;
    }

    public AsyncTask<String, Integer, Integer> execute() {
        FFplay.setAudioHandler(new GenericAudioHandler());
        FFplay.setControllerManager(new GenericControllerHandler());

        AudioHandler audioHandler = FFplay.getAudioHandler();
        if (audioHandler != null) {
            audioHandler.initialize();
        }
        ControllerHandler controllerHandler = FFplay.getControllerHandler();
        if (controllerHandler != null) {
            controllerHandler.initialize(context);
        }

        AsyncSingleFFplayExecuteTask task = new AsyncSingleFFplayExecuteTask(command);
        return task.execute("");
    }

}
