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
import android.util.DisplayMetrics;
import android.view.Surface;

public interface PlayerManager {

    void initialize();

    boolean setActivityTitle(final String title);

    void setWindowStyle(final boolean fullScreen);

    void setOrientation(final int w, final int h, final boolean resizable, final String hint);

    boolean isScreenKeyboardShown();

    boolean sendMessage(final int command, final int param);

    Context getContext();

    boolean isAndroidTV();

    DisplayMetrics getDisplayDPI();

    boolean getManifestEnvironmentVariables();

    boolean showTextInput(final int x, final int y, final int w, final int h);

    Surface getNativeSurface();

    int[] inputGetInputDeviceIds(final int sources);

    boolean clipboardHasText();

    String clipboardGetText();

    void clipboardSetText(final String string);

    int showMessageBox(final int flags, final String title, final String message, final int[] buttonFlags, final int[] buttonIds, final String[] buttonTexts, final int[] colors);

}
