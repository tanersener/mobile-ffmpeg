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

public interface AudioHandler {

    void initialize();

    int audioOpen(final int sampleRate, final boolean is16Bit, final boolean isStereo, final int desiredFrames);

    void audioWriteShortBuffer(final short[] buffer);

    void audioWriteByteBuffer(final byte[] buffer);

    int captureOpen(final int sampleRate, final boolean is16Bit, final boolean isStereo, final int desiredFrames);

    int captureReadShortBuffer(final short[] buffer, final boolean blocking);

    int captureReadByteBuffer(final byte[] buffer, final boolean blocking);

    void audioClose();

    void captureClose();

}
