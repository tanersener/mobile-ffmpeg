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
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg;

/**
 * <p>Helper enumeration type for Android ABIs; includes only supported ABIs.
 *
 * @author Taner Sener
 * @since v1.0
 */
public enum Abi {

    /**
     * Represents armeabi-v7a ABI with NEON support
     */
    ABI_ARMV7A_NEON("armeabi-v7a-neon"),

    /**
     * Represents armeabi-v7a ABI
     */
    ABI_ARMV7A("armeabi-v7a"),

    /**
     * Represents armeabi ABI
     */
    ABI_ARM("armeabi"),

    /**
     * Represents x86 ABI
     */
    ABI_X86("x86"),

    /**
     * Represents x86_64 ABI
     */
    ABI_X86_64("x86_64"),

    /**
     * Represents arm64-v8a ABI
     */
    ABI_ARM64_V8A("arm64-v8a"),

    /**
     * Represents not supported ABIs
     */
    ABI_UNKNOWN("unknown");

    private String name;

    /**
     * <p>Returns enumeration defined by ABI name.
     *
     * @param abiName ABI name
     * @return enumeration defined by ABI name
     */
    public static Abi from(final String abiName) {
        if (abiName == null) {
            return ABI_UNKNOWN;
        } else if (abiName.equals(ABI_ARM.getName())) {
            return ABI_ARM;
        } else if (abiName.equals(ABI_ARMV7A.getName())) {
            return ABI_ARMV7A;
        } else if (abiName.equals(ABI_ARMV7A_NEON.getName())) {
            return ABI_ARMV7A_NEON;
        } else if (abiName.equals(ABI_ARM64_V8A.getName())) {
            return ABI_ARM64_V8A;
        } else if (abiName.equals(ABI_X86.getName())) {
            return ABI_X86;
        } else if (abiName.equals(ABI_X86_64.getName())) {
            return ABI_X86_64;
        } else {
            return ABI_UNKNOWN;
        }
    }

    /**
     * Returns ABI name as defined in Android NDK documentation.
     *
     * @return ABI name
     */
    public String getName() {
        return name;
    }

    /**
     * Creates new enum.
     *
     * @param abiName ABI name
     */
    Abi(final String abiName) {
        this.name = abiName;
    }

}
