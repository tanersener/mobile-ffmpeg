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

package com.arthenica.mobileffmpeg;

/**
 * <p>Represents an ongoing FFmpeg execution.
 *
 * @author Taner Sener
 * @since v4.4
 */
public class FFmpegExecution {
    private final long executionId;
    private final String command;

    public FFmpegExecution(final long executionId, final String[] arguments) {
        this.executionId = executionId;
        this.command = FFmpeg.argumentsToString(arguments);
    }

    public long getExecutionId() {
        return executionId;
    }

    public String getCommand() {
        return command;
    }

}
