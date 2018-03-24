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
 * <p>Android ABI helper enum.
 *
 * @author Taner Sener
 */
public enum Abi {

    ABI_ARMV7A_NEON("armeabi-v7a-neon"),
    ABI_ARMV7A("armeabi-v7a"),
    ABI_ARM("armeabi"),
    ABI_X86("x86"),
    ABI_X86_64("x86_64"),
    ABI_ARM64_V8A("arm64-v8a"),
    ABI_UNKNOWN("unknown");

    private String value;

    public static Abi from(final String value) {
        if (value == null) {
            return ABI_UNKNOWN;
        } else if (value.equals(ABI_ARM.getValue())) {
            return ABI_ARM;
        } else if (value.equals(ABI_ARMV7A.getValue())) {
            return ABI_ARMV7A;
        } else if (value.equals(ABI_ARMV7A_NEON.getValue())) {
            return ABI_ARMV7A_NEON;
        } else if (value.equals(ABI_ARM64_V8A.getValue())) {
            return ABI_ARM64_V8A;
        } else if (value.equals(ABI_X86.getValue())) {
            return ABI_X86;
        } else if (value.equals(ABI_X86_64.getValue())) {
            return ABI_X86_64;
        } else {
            return ABI_UNKNOWN;
        }
    }

    public String getValue() {
        return value;
    }

    Abi(final String value) {
        this.value = value;
    }

}
