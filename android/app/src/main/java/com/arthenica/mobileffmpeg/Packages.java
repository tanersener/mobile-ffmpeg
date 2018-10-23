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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * <p>Provides helper methods to extract binary package information.
 *
 * @since 3.0
 */
class Packages {

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
        supportedExternalLibraries.add("mp3lame");
        supportedExternalLibraries.add("libaom");
        supportedExternalLibraries.add("libass");
        supportedExternalLibraries.add("iconv");
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
    }

    /**
     * Returns enabled external libraries by FFmpeg.
     *
     * @return enabled external libraries
     */
    static List<String> getExternalLibraries() {
        final String buildConfiguration = Config.getNativeBuildConf();

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

    /**
     * Returns MobileFFmpeg binary package name.
     *
     * @return guessed MobileFFmpeg binary package name
     */
    static String getPackageName() {
        final List<String> externalLibraryList = getExternalLibraries();
        final boolean speex = externalLibraryList.contains("speex");
        final boolean fribidi = externalLibraryList.contains("fribidi");
        final boolean gnutls = externalLibraryList.contains("gnutls");
        final boolean xvidcore = externalLibraryList.contains("xvidcore");

        boolean min = false;
        boolean minGpl = false;
        boolean https = false;
        boolean httpsGpl = false;
        boolean audio = false;
        boolean video = false;
        boolean full = false;
        boolean fullGpl = false;

        if (speex && fribidi) {
            if (xvidcore) {
                fullGpl = true;
            } else {
                full = true;
            }
        } else if (speex) {
            audio = true;
        } else if (fribidi) {
            video = true;
        } else if (xvidcore) {
            if (gnutls) {
                httpsGpl = true;
            } else {
                minGpl = true;
            }
        } else {
            if (gnutls) {
                https = true;
            } else {
                min = true;
            }
        }

        if (fullGpl) {
            if (externalLibraryList.contains("chromaprint") &&
                    externalLibraryList.contains("fontconfig") &&
                    externalLibraryList.contains("freetype") &&
                    externalLibraryList.contains("fribidi") &&
                    externalLibraryList.contains("gmp") &&
                    externalLibraryList.contains("gnutls") &&
                    externalLibraryList.contains("kvazaar") &&
                    externalLibraryList.contains("mp3lame") &&
                    externalLibraryList.contains("libaom") &&
                    externalLibraryList.contains("libass") &&
                    externalLibraryList.contains("iconv") &&
                    externalLibraryList.contains("libilbc") &&
                    externalLibraryList.contains("libtheora") &&
                    externalLibraryList.contains("libvidstab") &&
                    externalLibraryList.contains("libvorbis") &&
                    externalLibraryList.contains("libvpx") &&
                    externalLibraryList.contains("libwebp") &&
                    externalLibraryList.contains("libxml2") &&
                    externalLibraryList.contains("opencore-amr") &&
                    externalLibraryList.contains("opus") &&
                    externalLibraryList.contains("shine") &&
                    externalLibraryList.contains("sdl") &&
                    externalLibraryList.contains("snappy") &&
                    externalLibraryList.contains("soxr") &&
                    externalLibraryList.contains("speex") &&
                    externalLibraryList.contains("tesseract") &&
                    externalLibraryList.contains("twolame") &&
                    externalLibraryList.contains("wavpack") &&
                    externalLibraryList.contains("x264") &&
                    externalLibraryList.contains("x265") &&
                    externalLibraryList.contains("xvidcore")) {
                return "full-gpl";
            } else {
                return "custom";
            }
        }

        if (full) {
            if (externalLibraryList.contains("chromaprint") &&
                    externalLibraryList.contains("fontconfig") &&
                    externalLibraryList.contains("freetype") &&
                    externalLibraryList.contains("fribidi") &&
                    externalLibraryList.contains("gmp") &&
                    externalLibraryList.contains("gnutls") &&
                    externalLibraryList.contains("kvazaar") &&
                    externalLibraryList.contains("mp3lame") &&
                    externalLibraryList.contains("libaom") &&
                    externalLibraryList.contains("libass") &&
                    externalLibraryList.contains("iconv") &&
                    externalLibraryList.contains("libilbc") &&
                    externalLibraryList.contains("libtheora") &&
                    externalLibraryList.contains("libvorbis") &&
                    externalLibraryList.contains("libvpx") &&
                    externalLibraryList.contains("libwebp") &&
                    externalLibraryList.contains("libxml2") &&
                    externalLibraryList.contains("opencore-amr") &&
                    externalLibraryList.contains("opus") &&
                    externalLibraryList.contains("shine") &&
                    externalLibraryList.contains("sdl") &&
                    externalLibraryList.contains("snappy") &&
                    externalLibraryList.contains("soxr") &&
                    externalLibraryList.contains("speex") &&
                    externalLibraryList.contains("tesseract") &&
                    externalLibraryList.contains("twolame") &&
                    externalLibraryList.contains("wavpack")) {
                return "full";
            } else {
                return "custom";
            }
        }

        if (video) {
            if (externalLibraryList.contains("fontconfig") &&
                    externalLibraryList.contains("freetype") &&
                    externalLibraryList.contains("fribidi") &&
                    externalLibraryList.contains("kvazaar") &&
                    externalLibraryList.contains("libaom") &&
                    externalLibraryList.contains("libass") &&
                    externalLibraryList.contains("iconv") &&
                    externalLibraryList.contains("libtheora") &&
                    externalLibraryList.contains("libvpx") &&
                    externalLibraryList.contains("libwebp") &&
                    externalLibraryList.contains("snappy")) {
                return "video";
            } else {
                return "custom";
            }
        }

        if (audio) {
            if (externalLibraryList.contains("mp3lame") &&
                    externalLibraryList.contains("libilbc") &&
                    externalLibraryList.contains("libvorbis") &&
                    externalLibraryList.contains("opencore-amr") &&
                    externalLibraryList.contains("opus") &&
                    externalLibraryList.contains("shine") &&
                    externalLibraryList.contains("soxr") &&
                    externalLibraryList.contains("speex") &&
                    externalLibraryList.contains("twolame") &&
                    externalLibraryList.contains("wavpack")) {
                return "audio";
            } else {
                return "custom";
            }
        }

        if (httpsGpl) {
            if (externalLibraryList.contains("gmp") &&
                    externalLibraryList.contains("gnutls") &&
                    externalLibraryList.contains("libvidstab") &&
                    externalLibraryList.contains("x264") &&
                    externalLibraryList.contains("x265") &&
                    externalLibraryList.contains("xvidcore")) {
                return "https-gpl";
            } else {
                return "custom";
            }
        }

        if (https) {
            if (externalLibraryList.contains("gmp") &&
                    externalLibraryList.contains("gnutls")) {
                return "https";
            } else {
                return "custom";
            }
        }

        if (minGpl) {
            if (externalLibraryList.contains("libvidstab") &&
                    externalLibraryList.contains("x264") &&
                    externalLibraryList.contains("x265") &&
                    externalLibraryList.contains("xvidcore")) {
                return "min-gpl";
            } else {
                return "custom";
            }
        }

        return "min";
    }

}
