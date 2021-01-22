/*
 * Copyright (c) 2018-2020 Taner Sener
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
 * <p>This class is used to detect running ABI name using Google <code>cpu-features</code> library.
 *
 * @author Taner Sener
 * @since v1.0
 */
public class AbiDetect {

    static {
        armV7aNeonLoaded = false;

        System.loadLibrary("mobileffmpeg_abidetect");

        /* ALL LIBRARIES LOADED AT STARTUP */
        Config.class.getName();
        FFmpeg.class.getName();
    }

    static final String ARM_V7A = "arm-v7a";

    static final String ARM_V7A_NEON = "arm-v7a-neon";

    private static boolean armV7aNeonLoaded;

    /**
     * Default constructor hidden.
     */
    private AbiDetect() {
    }

    static void setArmV7aNeonLoaded(final boolean armV7aNeonLoaded) {
        AbiDetect.armV7aNeonLoaded = armV7aNeonLoaded;
    }

    /**
     * <p>Returns loaded ABI name.
     *
     * @return loaded ABI name
     */
    public static String getAbi() {
        if (armV7aNeonLoaded) {
            return ARM_V7A_NEON;
        } else {
            return getNativeAbi();
        }
    }

    /**
     * <p>Returns loaded ABI name.
     *
     * @return loaded ABI name
     */
    public native static String getNativeAbi();

    /**
     * <p>Returns ABI name of the running cpu.
     *
     * @return ABI name of the running cpu
     */
    public native static String getNativeCpuAbi();

    /**
     * <p>Returns whether MobileFFmpeg release is a long term release or not.
     *
     * @return yes or no
     */
    native static boolean isNativeLTSBuild();

    /**
     * <p>Returns build configuration for <code>FFmpeg</code>.
     *
     * @return build configuration string
     */
    native static String getNativeBuildConf();

}
