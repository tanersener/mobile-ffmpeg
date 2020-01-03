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
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg;

import android.util.Log;

/**
 * <p>Main class for FFprobe operations. Provides {@link #execute(String...)} method to execute
 * FFprobe commands.
 * <pre>
 *      int rc = FFprobe.execute("-hide_banner -v error -show_entries format=size -of default=noprint_wrappers=1 file1.mp4");
 *      Log.i(Config.TAG, String.format("Command execution %s.", (rc == 0?"completed successfully":"failed with rc=" + rc));
 * </pre>
 *
 * @author Taner Sener
 * @since v4.3.1
 */
public class FFprobe {

    static {
        AbiDetect.class.getName();
        Config.class.getName();
    }

    /**
     * Default constructor hidden.
     */
    private FFprobe() {
    }

    /**
     * <p>Synchronously executes FFprobe with arguments provided.
     *
     * @param arguments FFprobe command options/arguments as string array
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public static int execute(final String[] arguments) {
        final int lastReturnCode = Config.nativeFFprobeExecute(arguments);

        Config.setLastReturnCode(lastReturnCode);

        return lastReturnCode;
    }

    /**
     * <p>Synchronously executes FFprobe command provided. Space character is used to split command
     * into arguments. You can use single and double quote characters to specify arguments inside
     * your command.
     *
     * @param command FFprobe command
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public static int execute(final String command) {
        return execute(FFmpeg.parseArguments(command));
    }

    /**
     * <p>Returns media information for given file.
     *
     * <p>This method does not support executing multiple concurrent operations. If you execute
     * multiple operations (execute or getMediaInformation) at the same time, the response of this
     * method is not predictable.
     *
     * @param path path or uri of media file
     * @return media information
     * @since 3.0
     */
    public static MediaInformation getMediaInformation(final String path) {
        final int rc = execute(new String[]{"-v", "info", "-hide_banner", "-i", path});

        if (rc == 0) {
            return MediaInformationParser.from(Config.getLastCommandOutput());
        } else {
            Log.i(Config.TAG, Config.getLastCommandOutput());
            return null;
        }
    }

    /**
     * <p>Returns media information for given file.
     *
     * <p>This method does not support executing multiple concurrent operations. If you execute
     * multiple operations (execute or getMediaInformation) at the same time, the response of this
     * method is not predictable.
     *
     * @param path    path or uri of media file
     * @param timeout complete timeout
     * @return media information
     * @since 3.0
     * @deprecated this method is deprecated since v4.3.1. You can still use this method but
     * <code>timeout</code> parameter is not effective anymore.
     */
    public static MediaInformation getMediaInformation(final String path, final Long timeout) {
        return getMediaInformation(path);
    }

}
