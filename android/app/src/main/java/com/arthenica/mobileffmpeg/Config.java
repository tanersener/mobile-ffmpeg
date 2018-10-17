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

import android.content.Context;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

import static com.arthenica.mobileffmpeg.FFmpeg.getVersion;

/**
 * <p>This class is used to configure MobileFFmpeg library utilities/tools.
 *
 * <p>1. {@link LogCallback}: By default this class redirects FFmpeg output to Logcat. As another
 * option, it is possible not to print messages to Logcat and pass them to a {@link LogCallback}
 * function. This function can decide whether to print these logs, show them inside another
 * container or ignore them.
 *
 * <p>2. {@link #setLogLevel(Level)}/{@link #getLogLevel()}: Use this methods to see/control FFmpeg
 * log severity.
 *
 * <p>3. {@link StatisticsCallback}: It is possible to receive statistics about ongoing operation by
 * defining a {@link StatisticsCallback} function or by calling {@link #getLastReceivedStatistics()}
 * method.
 *
 * <p>4. Font configuration: It is possible to register custom fonts with
 * {@link #setFontconfigConfigurationPath(String)} and
 * {@link #setFontDirectory(Context, String, Map)} methods.
 *
 * <p>PS: This class is introduced in v2.1 as an enhanced version of older <code>Log</code> class.
 *
 * @author Taner Sener
 * @since v2.1
 */
public class Config {

    /**
     * Defines tag used for logging.
     */
    public static final String TAG = "mobile-ffmpeg";

    private static LogCallback logCallbackFunction;

    private static Level activeLogLevel;

    private static StatisticsCallback statisticsCallbackFunction;

    private static Statistics lastReceivedStatistics;

    private static StringBuffer systemCommandOutput = new StringBuffer();

    private static boolean runningSystemCommand = false;

    private static final List<String> supportedExternalLibraries;

    static {
        supportedExternalLibraries = new ArrayList<>();
        supportedExternalLibraries.add("chromaprint");
        supportedExternalLibraries.add("fontconfig");
        supportedExternalLibraries.add("freetype");
        supportedExternalLibraries.add("fribidi");
        supportedExternalLibraries.add("gmp");
        supportedExternalLibraries.add("gnutls");
        supportedExternalLibraries.add("kvazaar");
        supportedExternalLibraries.add("lame");
        supportedExternalLibraries.add("libaom");
        supportedExternalLibraries.add("libass");
        supportedExternalLibraries.add("libiconv");
        supportedExternalLibraries.add("libilbc");
        supportedExternalLibraries.add("libtheora");
        supportedExternalLibraries.add("libvidstab");
        supportedExternalLibraries.add("libvorbis");
        supportedExternalLibraries.add("libvpx");
        supportedExternalLibraries.add("libwebp");
        supportedExternalLibraries.add("libxml2");
        supportedExternalLibraries.add("opencore-amr");
        supportedExternalLibraries.add("opus");
        supportedExternalLibraries.add("shine");
        supportedExternalLibraries.add("sdl");
        supportedExternalLibraries.add("snappy");
        supportedExternalLibraries.add("soxr");
        supportedExternalLibraries.add("speex");
        supportedExternalLibraries.add("tesseract");
        supportedExternalLibraries.add("twolame");
        supportedExternalLibraries.add("wavpack");
        supportedExternalLibraries.add("x264");
        supportedExternalLibraries.add("x265");
        supportedExternalLibraries.add("xvidcore");
        supportedExternalLibraries.add("android-zlib");
        supportedExternalLibraries.add("android-media-codec");

        Log.i(Config.TAG, "Loading mobile-ffmpeg.");

        /* ALL LIBRARIES LOADED AT STARTUP */
        String abiName = AbiDetect.getAbi();
        Abi abi = Abi.from(abiName);
        FFmpeg.class.getName();

        /*
         * NEON supported arm-v7a library has a different name
         */
        boolean nativeLibraryLoaded = false;
        if (abi == Abi.ABI_ARMV7A_NEON) {
            try {
                System.loadLibrary("mobileffmpeg-armv7a-neon");
                nativeLibraryLoaded = true;
            } catch (final UnsatisfiedLinkError e) {
                Log.i(Config.TAG, "NEON supported armeabi-v7a library not found. Loading default armeabi-v7a library.", e);
                abi = Abi.ABI_ARMV7A;
            }
        }

        if (!nativeLibraryLoaded) {
            System.loadLibrary("mobileffmpeg");
        }

        Log.i(Config.TAG, String.format("Loaded mobile-ffmpeg-%s-%s-%s.", getPackageName(), abi.getName(), getVersion()));

        /* NATIVE LOG LEVEL IS RECEIVED ONLY ON STARTUP */
        activeLogLevel = Level.from(getNativeLogLevel());

        lastReceivedStatistics = new Statistics();

        Config.enableRedirection();
    }

    /**
     * Default constructor hidden.
     */
    private Config() {
    }

    /**
     * <p>Enables log and statistics redirection.
     * <p>When redirection is not enabled FFmpeg logs are printed to stderr. By enabling redirection, they are routed
     * to Logcat and can be routed further to a callback function.
     * <p>Statistics redirection behaviour is similar. Statistics are not printed at all if redirection is not enabled.
     * If it is enabled then it is possible to define a statistics callback function but if you don't, they are not
     * printed anywhere and only saved as <code>lastReceivedStatistics</code> data which can be polled with
     * {@link #getLastReceivedStatistics()}.
     * <p>Note that redirection is enabled by default. If you do not want to use its functionality please use
     * {@link #disableRedirection()} to disable it.
     */
    public static void enableRedirection() {
        enableNativeRedirection();
    }

    /**
     * <p>Disables log and statistics redirection.
     */
    public static void disableRedirection() {
        disableNativeRedirection();
    }

    /**
     * Returns log level.
     *
     * @return log level
     */
    public static Level getLogLevel() {
        return activeLogLevel;
    }

    /**
     * Sets log level.
     *
     * @param level log level
     */
    public static void setLogLevel(final Level level) {
        if (level != null) {
            activeLogLevel = level;
            setNativeLogLevel(level.getValue());
        }
    }

    /**
     * <p>Sets a callback function to redirect FFmpeg logs.
     *
     * @param newLogCallback new log callback function or NULL to disable a previously defined callback
     */
    public static void enableLogCallback(final LogCallback newLogCallback) {
        logCallbackFunction = newLogCallback;
    }

    /**
     * <p>Sets a callback function to redirect FFmpeg statistics.
     *
     * @param statisticsCallback new statistics callback function or NULL to disable a previously defined callback
     */
    public static void enableStatisticsCallback(final StatisticsCallback statisticsCallback) {
        statisticsCallbackFunction = statisticsCallback;
    }

    /**
     * <p>Log redirection method called by JNI/native part.
     *
     * @param levelValue log level as defined in {@link Level}
     * @param logMessage redirected log message
     */
    private static void log(final int levelValue, final byte[] logMessage) {
        final Level level = Level.from(levelValue);
        final String text = new String(logMessage);

        if (runningSystemCommand) {
            systemCommandOutput.append(text);
            return;
        }

        // ALWAYS REDIRECT COMMAND OUTPUT
        FFmpeg.appendCommandOutput(text);

        if (activeLogLevel == Level.AV_LOG_QUIET || levelValue > activeLogLevel.getValue()) {
            // LOG NEITHER PRINTED NOR FORWARDED
            return;
        }

        if (logCallbackFunction != null) {
            logCallbackFunction.apply(new LogMessage(level, text));
        } else {
            switch (level) {
                case AV_LOG_QUIET: {
                    // PRINT NO OUTPUT
                }
                break;
                case AV_LOG_TRACE:
                case AV_LOG_DEBUG: {
                    android.util.Log.d(TAG, text);
                }
                break;
                case AV_LOG_VERBOSE: {
                    android.util.Log.v(TAG, text);
                }
                break;
                case AV_LOG_INFO: {
                    android.util.Log.i(TAG, text);
                }
                break;
                case AV_LOG_WARNING: {
                    android.util.Log.w(TAG, text);
                }
                break;
                case AV_LOG_ERROR:
                case AV_LOG_FATAL:
                case AV_LOG_PANIC: {
                    android.util.Log.e(TAG, text);
                }
                break;
                default: {
                    android.util.Log.v(TAG, text);
                }
                break;
            }
        }
    }

    /**
     * <p>Statistics redirection method called by JNI/native part.
     *
     * @param videoFrameNumber last processed frame number for videos
     * @param videoFps         frames processed per second for videos
     * @param videoQuality     quality of the video stream
     * @param size             size in bytes
     * @param time             processed duration in milliseconds
     * @param bitrate          output bit rate in kbits/s
     * @param speed            processing speed = processed duration / operation duration
     */
    private static void statistics(final int videoFrameNumber, final float videoFps,
                                   final float videoQuality, final long size, final int time,
                                   final double bitrate, final double speed) {
        final Statistics newStatistics = new Statistics(videoFrameNumber, videoFps, videoQuality, size, time, bitrate, speed);
        lastReceivedStatistics.update(newStatistics);

        if (statisticsCallbackFunction != null) {
            statisticsCallbackFunction.apply(lastReceivedStatistics);
        }
    }

    /**
     * <p>Returns the last received statistics data.
     *
     * @return last received statistics data
     */
    public static Statistics getLastReceivedStatistics() {
        return lastReceivedStatistics;
    }

    /**
     * <p>Resets last received statistics. It is recommended to call it before starting a new execution.
     */
    public static void resetStatistics() {
        lastReceivedStatistics = new Statistics();
    }

    /**
     * <p>Sets and overrides <code>fontconfig</code> configuration directory.
     *
     * @param path directory which contains fontconfig configuration (fonts.conf)
     * @throws ErrnoException if an error occurs
     */
    public static void setFontconfigConfigurationPath(final String path) throws ErrnoException {
        Os.setenv("FONTCONFIG_PATH", path, true);
    }

    /**
     * <p>Registers fonts inside the given path, so they are available to use in FFmpeg filters.
     * <p>
     * <p>Note that you need to build <code>MobileFFmpeg</code> with <code>fontconfig</code>
     * enabled or use a prebuilt package with <code>fontconfig</code> inside to use this feature.
     *
     * @param context           application context to access application data
     * @param fontDirectoryPath directory which contains fonts (.ttf and .otf files)
     * @param fontNameMapping   custom font name mappings, useful to access your fonts with more friendly names
     */
    public static void setFontDirectory(final Context context, final String fontDirectoryPath, final Map<String, String> fontNameMapping) {
        final File cacheDir = context.getCacheDir();
        int validFontNameMappingCount = 0;

        final File tempConfigurationDirectory = new File(cacheDir, ".mobileffmpeg");
        if (!tempConfigurationDirectory.exists()) {
            boolean tempFontConfDirectoryCreated = tempConfigurationDirectory.mkdirs();
            Log.d(TAG, String.format("Created temporary font conf directory: %s.", tempFontConfDirectoryCreated));
        }

        final File fontConfiguration = new File(tempConfigurationDirectory, "fonts.conf");
        if (fontConfiguration.exists()) {
            boolean fontConfigurationDeleted = fontConfiguration.delete();
            Log.d(TAG, String.format("Deleted old temporary font configuration: %s.", fontConfigurationDeleted));
        }

        /* PROCESS MAPPINGS FIRST */
        final StringBuilder fontNameMappingBlock = new StringBuilder("");
        if (fontNameMapping != null && (fontNameMapping.size() > 0)) {
            fontNameMapping.entrySet();
            for (Map.Entry<String, String> mapping : fontNameMapping.entrySet()) {
                String fontName = mapping.getKey();
                String mappedFontName = mapping.getValue();

                if ((fontName != null) && (mappedFontName != null) && (fontName.trim().length() > 0) && (mappedFontName.trim().length() > 0)) {
                    fontNameMappingBlock.append("        <match target=\"pattern\">\n");
                    fontNameMappingBlock.append("                <test qual=\"any\" name=\"family\">\n");
                    fontNameMappingBlock.append(String.format("                        <string>%s</string>\n", fontName));
                    fontNameMappingBlock.append("                </test>\n");
                    fontNameMappingBlock.append("                <edit name=\"family\" mode=\"assign\" binding=\"same\">\n");
                    fontNameMappingBlock.append(String.format("                        <string>%s</string>\n", mappedFontName));
                    fontNameMappingBlock.append("                </edit>\n");
                    fontNameMappingBlock.append("        </match>\n");

                    validFontNameMappingCount++;
                }
            }
        }

        final String fontConfig = "<?xml version=\"1.0\"?>\n" +
                "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n" +
                "<fontconfig>\n" +
                "    <dir>.</dir>\n" +
                "    <dir>" + fontDirectoryPath + "</dir>\n" +
                fontNameMappingBlock +
                "</fontconfig>";

        final AtomicReference<FileOutputStream> reference = new AtomicReference<>();
        try {
            final FileOutputStream outputStream = new FileOutputStream(fontConfiguration);
            reference.set(outputStream);

            outputStream.write(fontConfig.getBytes());
            outputStream.flush();

            Log.d(TAG, String.format("Saved new temporary font configuration with %d font name mappings.", validFontNameMappingCount));

            setFontconfigConfigurationPath(tempConfigurationDirectory.getAbsolutePath());

            Log.d(TAG, String.format("Font directory %s registered successfully.", fontDirectoryPath));

        } catch (final ErrnoException | IOException e) {
            Log.e(TAG, String.format("Failed to set font directory: %s.", fontDirectoryPath), e);
        } finally {
            if (reference.get() != null) {
                try {
                    reference.get().close();
                } catch (IOException e) {
                    // DO NOT PRINT THIS ERROR
                }
            }
        }
    }

    /**
     * <p>Returns package name.
     *
     * @return guessed package name according to supported external libraries
     * @since 3.0
     */
    public static String getPackageName() {
        final List<String> externalLibraryList = getExternalLibraries();
        final boolean speex = externalLibraryList.contains("speex");
        final boolean fribidi = externalLibraryList.contains("fribidi");
        final boolean gnutls = externalLibraryList.contains("gnutls");
        final boolean xvidcore = externalLibraryList.contains("xvidcore");

        if (speex && fribidi) {
            if (xvidcore) {
                return "full-gpl";
            } else {
                return "full";
            }
        } else if (speex) {
            return "audio";
        } else if (fribidi) {
            return "video";
        } else if (xvidcore) {
            if (gnutls) {
                return "https-gpl";
            } else {
                return "min-gpl";
            }
        } else {
            if (gnutls) {
                return "https";
            } else {
                return "min";
            }
        }

    }

    /**
     * <p>Returns supported external libraries.
     *
     * @return list of supported external libraries
     * @since 3.0
     */
    public static List<String> getExternalLibraries() {
        final String buildConfiguration = getNativeBuildConf();

        final List<String> enabledLibraryList = new ArrayList<>();
        for (String supportedExternalLibrary : supportedExternalLibraries) {
            if (buildConfiguration.contains("enable-" + supportedExternalLibrary) ||
                    buildConfiguration.contains("enable-lib" + supportedExternalLibrary)) {
                enabledLibraryList.add(supportedExternalLibrary);
            }
        }

        Collections.sort(enabledLibraryList);

        return enabledLibraryList;
    }

    static int systemExecute(final String[] arguments) {
        runningSystemCommand = true;

        int rc = Config.nativeExecute(arguments);

        runningSystemCommand = false;

        return rc;
    }

    /**
     * <p>Enables native redirection. Necessary for log and statistics callback functions.
     */
    private static native void enableNativeRedirection();

    /**
     * <p>Disables native redirection
     */
    private static native void disableNativeRedirection();

    /**
     * Sets native log level
     *
     * @param level log level
     */
    private static native void setNativeLogLevel(int level);

    /**
     * Returns native log level.
     *
     * @return log level
     */
    private static native int getNativeLogLevel();

    /**
     * <p>Returns FFmpeg version bundled within the library natively.
     *
     * @return FFmpeg version
     */
    native static String getNativeFFmpegVersion();

    /**
     * <p>Returns MobileFFmpeg library version natively.
     *
     * @return MobileFFmpeg version
     */
    native static String getNativeVersion();

    /**
     * <p>Synchronously executes FFmpeg natively with arguments provided.
     *
     * @param arguments FFmpeg command options/arguments as string array
     * @return zero on successful execution, 255 on user cancel and non-zero on error
     */
    native static int nativeExecute(final String[] arguments);

    /**
     * <p>Cancels an ongoing operation natively. This function does not wait for termination to
     * complete and returns immediately.
     */
    native static void nativeCancel();

    /**
     * <p>Returns build configuration for <code>FFmpeg</code>.
     *
     * @return build configuration string
     */
    native static String getNativeBuildConf();

}
