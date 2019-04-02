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

import android.util.Log;

import java.util.Arrays;

/**
 * <p>Main class for FFmpeg operations. Provides {@link #execute(String...)} method to execute
 * FFmpeg commands.
 * <pre>
 *      int rc = FFmpeg.execute("-i", "file1.mp4", "-c:v", "libxvid", "file1.avi");
 *      Log.i(Config.TAG, String.format("Command execution %s.", (rc == 0?"completed successfully":"failed with rc=" + rc));
 * </pre>
 *
 * @author Taner Sener
 * @since v1.0
 */
public class FFmpeg {

    public static final int RETURN_CODE_SUCCESS = 0;

    public static final int RETURN_CODE_CANCEL = 255;

    private static int lastReturnCode = 0;

    private static StringBuffer lastCommandOutput = new StringBuffer();

    static {
        AbiDetect.class.getName();
        Config.class.getName();
    }

    /**
     * Default constructor hidden.
     */
    private FFmpeg() {
    }

    /**
     * <p>Appends given log output to the last command output.
     *
     * @param output log output
     */
    static void appendCommandOutput(final String output) {
        lastCommandOutput.append(output);
    }

    /**
     * <p>Returns FFmpeg version bundled within the library.
     *
     * @return FFmpeg version
     */
    public static String getFFmpegVersion() {
        return Config.getNativeFFmpegVersion();
    }

    /**
     * <p>Returns MobileFFmpeg library version.
     *
     * @return MobileFFmpeg version
     */
    public static String getVersion() {
        if (AbiDetect.isNativeLTSBuild()) {
            return String.format("%s-lts", Config.getNativeVersion());
        } else {
            return Config.getNativeVersion();
        }
    }

    /**
     * <p>Synchronously executes FFmpeg with arguments provided.
     *
     * @param arguments FFmpeg command options/arguments as string array
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public static int execute(final String[] arguments) {
        lastCommandOutput = new StringBuffer();

        lastReturnCode = Config.nativeExecute(arguments);

        return lastReturnCode;
    }

    /**
     * <p>Synchronously executes FFmpeg command provided. Command is split into arguments using
     * provided delimiter.
     *
     * @param command   FFmpeg command
     * @param delimiter delimiter used between arguments
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     * @since 3.0
     */
    public static int execute(final String command, final String delimiter) {
        return execute((command == null) ? new String[]{""} : command.split((delimiter == null) ? " " : delimiter));
    }

    /**
     * <p>Synchronously executes FFmpeg command provided. Space character is used to split command
     * into arguments.
     *
     * @param command FFmpeg command
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    public static int execute(final String command) {
        return execute(command, " ");
    }

    /**
     * <p>Cancels an ongoing operation. This function does not wait for termination to complete and
     * returns immediately.
     */
    public static void cancel() {
        Config.nativeCancel();
    }

    /**
     * <p>Returns return code of last executed command.
     *
     * @return return code of last executed command
     * @since 3.0
     */
    public static int getLastReturnCode() {
        return lastReturnCode;
    }

    /**
     * <p>Returns log output of last executed command. Please note that disabling redirection using
     * {@link Config#disableRedirection()} method also disables this functionality.
     *
     * @return output of last executed command
     * @since 3.0
     */
    public static String getLastCommandOutput() {
        return lastCommandOutput.toString();
    }

    /**
     * <p>Returns media information for given file.
     *
     * @param path path or uri of media file
     * @return media information
     * @since 3.0
     */
    public static MediaInformation getMediaInformation(final String path) {
        return getMediaInformation(path, 10000L);
    }

    /**
     * <p>Returns media information for given file.
     *
     * @param path    path or uri of media file
     * @param timeout complete timeout
     * @return media information
     * @since 3.0
     */
    public static MediaInformation getMediaInformation(final String path, final Long timeout) {
        int rc = Config.systemExecute(new String[]{"-v", "info", "-hide_banner", "-i", path, "-f", "null", "-"}, Arrays.asList("Press [q] to stop, [?] for help", "No such file or directory", "Input/output error", "Conversion failed"), timeout);

        if (rc == 0) {
            return MediaInformationParser.from(Config.getSystemCommandOutput());
        } else {
            Log.i(Config.TAG, Config.getSystemCommandOutput());
            return null;
        }
    }

    /**
     * <p>Returns whether MobileFFmpeg release is a long term release or not.
     *
     * @return YES or NO
     */
    public static boolean isLTSBuild() {
        return AbiDetect.isNativeLTSBuild();
    }

    /**
     * <p>Returns MobileFFmpeg library build date.
     *
     * @return MobileFFmpeg library build date
     */
    public static String getBuildDate() {
        return Config.getNativeBuildDate();
    }

}
