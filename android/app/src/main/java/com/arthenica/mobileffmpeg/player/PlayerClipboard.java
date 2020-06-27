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
 * - SDLClipboardHandler renamed as PlayerClipboard
 * - Added null checks for clipboardManager field
 */

package com.arthenica.mobileffmpeg.player;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;

import com.arthenica.mobileffmpeg.FFplay;

/**
 * <p>Clipboard for FFplay Player.
 */
public class PlayerClipboard implements ClipboardManager.OnPrimaryClipChangedListener {

    protected final Context context;
    protected final ClipboardManager clipboardManager;

    public PlayerClipboard(final Context context) {
        this.context = context;
        this.clipboardManager = (android.content.ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);

        if (this.clipboardManager != null) {
            this.clipboardManager.addPrimaryClipChangedListener(this);
        }
    }

    public boolean clipboardHasText() {
        if (clipboardManager != null) {
            final ClipData clip = clipboardManager.getPrimaryClip();
            return (clip != null) && (clip.getItemCount() > 0);
        }

        return false;
    }

    public String clipboardGetText() {
        CharSequence text = null;

        if (clipboardManager != null) {
            ClipData clip = clipboardManager.getPrimaryClip();
            if (clip != null && clip.getItemCount() > 0) {
                text = clip.getItemAt(0).coerceToText(context);
            }
        }

        if (text != null) {
            return text.toString();
        } else {
            return null;
        }
    }

    public void clipboardSetText(final String string) {
        if (clipboardManager != null) {
            clipboardManager.removePrimaryClipChangedListener(this);
            clipboardManager.setPrimaryClip(ClipData.newPlainText(null, string));
            clipboardManager.addPrimaryClipChangedListener(this);
        }
    }

    @Override
    public void onPrimaryClipChanged() {
        FFplay.playerOnClipboardChanged();
    }

}
