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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * <p>Main class for FFmpeg operations. Provides {@link #execute(String...)} method to execute
 * FFmpeg commands.
 * <pre>
 *      int rc = FFmpeg.execute("-i file1.mp4 -c:v libxvid file1.avi");
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
        lastReturnCode = Config.nativeExecute(arguments);
        return lastReturnCode;
    }

    /**
     * <p>Synchronously executes FFmpeg command provided. Command is split into arguments using
     * provided delimiter character.
     *
     * @param command   FFmpeg command
     * @param delimiter delimiter used to split arguments
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     * @since 3.0
     * @deprecated argument splitting mechanism used in this method is pretty simple and prone to
     * errors. Consider using a more advanced method like {@link #execute(String)} or
     * {@link #execute(String[])}
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
        return execute(parseArguments(command));
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
     * <p>Returns log output of last executed single command.
     *
     * <p>This method does not support executing multiple concurrent commands. If you execute
     * multiple commands at the same time, this method will return output from all executions.
     *
     * <p>Please note that disabling redirection using {@link Config#disableRedirection()} method
     * also disables this functionality.
     *
     * @return output of the last executed command
     * @since 3.0
     */
    public static String getLastCommandOutput() {
        String nativeLastCommandOutput = Config.getNativeLastCommandOutput();
        if (nativeLastCommandOutput != null) {

            // REPLACING CH(13) WITH CH(10)
            nativeLastCommandOutput = nativeLastCommandOutput.replace('\r', '\n');
        }
        return nativeLastCommandOutput;
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
        return getMediaInformation(path, 10000L);
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
     */
    public static MediaInformation getMediaInformation(final String path, final Long timeout) {
        final int rc = Config.systemExecute(new String[]{"-v", "info", "-hide_banner", "-i", path}, new ArrayList<>(Arrays.asList("Press [q] to stop, [?] for help", "No such file or directory", "Input/output error", "Conversion failed", "HTTP error")), "At least one output file must be specified", timeout);

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

    /**
     * <p>Parses the given command into arguments.
     *
     * @param command string command
     * @return array of arguments
     */
    static String[] parseArguments(final String command) {
        final List<String> argumentList = new ArrayList<>();
        StringBuilder currentArgument = new StringBuilder();

        boolean singleQuoteStarted = false;
        boolean doubleQuoteStarted = false;

        for (int i = 0; i < command.length(); i++) {
            final Character previousChar;
            if (i > 0) {
                previousChar = command.charAt(i - 1);
            } else {
                previousChar = null;
            }
            final char currentChar = command.charAt(i);

            if (currentChar == ' ') {
                if (singleQuoteStarted || doubleQuoteStarted) {
                    currentArgument.append(currentChar);
                } else if (currentArgument.length() > 0) {
                    argumentList.add(currentArgument.toString());
                    currentArgument = new StringBuilder();
                }
            } else if (currentChar == '\'' && (previousChar == null || previousChar != '\\')) {
                if (singleQuoteStarted) {
                    singleQuoteStarted = false;
                } else if (doubleQuoteStarted) {
                    currentArgument.append(currentChar);
                } else {
                    singleQuoteStarted = true;
                }
            } else if (currentChar == '\"' && (previousChar == null || previousChar != '\\')) {
                if (doubleQuoteStarted) {
                    doubleQuoteStarted = false;
                } else if (singleQuoteStarted) {
                    currentArgument.append(currentChar);
                } else {
                    doubleQuoteStarted = true;
                }
            } else {
                currentArgument.append(currentChar);
            }
        }

        if (currentArgument.length() > 0) {
            argumentList.add(currentArgument.toString());
        }

        return argumentList.toArray(new String[0]);
    }

    /**
     * <p>Prints the output of the last executed command to the logcat at the specified priority.
     *
     * @param logPriority one of {@link Log#VERBOSE}, {@link Log#DEBUG}, {@link Log#INFO},
     *                    {@link Log#WARN}, {@link Log#ERROR}, {@link Log#ASSERT}
     * @since 4.3
     */
    public static void printLastCommandOutput(int logPriority) {
        final int LOGGER_ENTRY_MAX_LEN = 4 * 1000;

        String buffer = FFmpeg.getLastCommandOutput();
        do {
            if (buffer.length() <= LOGGER_ENTRY_MAX_LEN) {
                Log.println(logPriority, Config.TAG, buffer);
                buffer = "";
            } else {
                final int index = buffer.substring(0, LOGGER_ENTRY_MAX_LEN).lastIndexOf('\n');
                if (index < 0) {
                    Log.println(logPriority, Config.TAG, buffer.substring(0, LOGGER_ENTRY_MAX_LEN));
                    buffer = buffer.substring(LOGGER_ENTRY_MAX_LEN);
                } else {
                    Log.println(logPriority, Config.TAG, buffer.substring(0, index));
                    buffer = buffer.substring(index);
                }
            }
        } while (buffer.length() > 0);
    }

}
