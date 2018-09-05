/*
 * Copyright (c) 2018 Taner Sener
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

package com.arthenica.mobileffmpeg;

/**
 * <p>This class is used to detect running ABI name using Android's <code>cpufeatures</code>
 * library.
 *
 * @author Taner Sener
 * @since v1.0
 */
public class AbiDetect {

    static {
        System.loadLibrary("mobileffmpeg-abidetect");

        /* ALL LIBRARIES LOADED AT STARTUP */
        Config.class.getName();
        FFmpeg.class.getName();
    }

    /**
     * Default constructor hidden.
     */
    private AbiDetect() {
    }

    /**
     * <p>Returns running ABI name.
     *
     * @return running ABI name
     */
    public native static String getAbi();

}
