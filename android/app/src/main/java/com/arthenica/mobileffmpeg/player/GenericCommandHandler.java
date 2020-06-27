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

package com.arthenica.mobileffmpeg.player;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

import static com.arthenica.mobileffmpeg.Config.TAG;

/**
 * A Handler class for Messages from native SDL applications.
 * It uses current Activities as target (e.g. for the title).
 * static to prevent implicit references to enclosing object.
 */
public class GenericCommandHandler extends Handler {

    public static final int COMMAND_CHANGE_TITLE = 1;
    public static final int COMMAND_CHANGE_WINDOW_STYLE = 2;
    public static final int COMMAND_SET_KEEP_SCREEN_ON = 5;

    protected final Context context;

    public GenericCommandHandler(final Context context) {
        this.context = context;
    }

    @Override
    public void handleMessage(final Message message) {
        switch (message.arg1) {
            case COMMAND_CHANGE_TITLE:
                if (context instanceof Activity) {
                    ((Activity) context).setTitle((String) message.obj);
                } else {
                    Log.e(TAG, "CommandHandler error handling message, getContext() returned no Activity.");
                }
                break;
            case COMMAND_CHANGE_WINDOW_STYLE:
                if (Build.VERSION.SDK_INT < 19) {
                    // This version of Android doesn't support the immersive fullscreen mode
                    break;
                }
/* This needs more testing, per bug 4096 - Enabling fullscreen on Android causes the app to toggle fullscreen mode continuously in a loop
 ***
                if (context instanceof Activity) {
                    Window window = ((Activity) context).getWindow();
                    if (window != null) {
                        if ((message.obj instanceof Integer) && (((Integer) message.obj).intValue() != 0)) {
                            int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                                        View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                                        View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                                        View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                                        View.SYSTEM_UI_FLAG_FULLSCREEN |
                                        View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
                            window.getDecorView().setSystemUiVisibility(flags);
                            window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                        } else {
                            int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
                            window.getDecorView().setSystemUiVisibility(flags);
                            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                        }
                    }
                } else {
                    Log.e(TAG, "error handling message, getContext() returned no Activity");
                }
***/
                break;
            case COMMAND_SET_KEEP_SCREEN_ON: {
                if (context instanceof Activity) {
                    Window window = ((Activity) context).getWindow();
                    if (window != null) {
                        if ((message.obj instanceof Integer) && (((Integer) message.obj).intValue() != 0)) {
                            window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                        } else {
                            window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                        }
                    }
                }
                break;
            }
            default:
                Log.e(TAG, String.format("CommandHandler error handling message, command is %d.", message.arg1));
        }
    }

}
