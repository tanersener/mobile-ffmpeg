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

import android.support.v4.util.Pair;
import android.util.Log;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public class MediaInformation {

    public static SimpleDateFormat DURATION_FORMAT;

    public static Date REFERENCE_DURATION;

    public static SimpleDateFormat START_TIME_FORMAT;

    public static Date REFERENCE_START_TIME;

    static {
        try {
            DURATION_FORMAT = new SimpleDateFormat("kk:mm:ss.SS", Locale.getDefault());
            REFERENCE_DURATION = DURATION_FORMAT.parse("00:00:00.00");
        } catch (final ParseException e) {
            Log.i(Config.TAG, "Preparing duration reference failed.", e);
            DURATION_FORMAT = null;
            REFERENCE_DURATION = null;
        }

        try {
            START_TIME_FORMAT = new SimpleDateFormat("s.SSSSSS", Locale.getDefault());
            REFERENCE_START_TIME = START_TIME_FORMAT.parse("0.000000");
        } catch (final ParseException e) {
            Log.i(Config.TAG, "Preparing start time reference failed.", e);
            START_TIME_FORMAT = null;
            REFERENCE_START_TIME = null;
        }
    }

    /**
     * Format
     */
    private String format;

    /**
     * Path
     */
    private String path;

    /**
     * Duration, in milliseconds
     */
    private String duration;

    /**
     * Start time, in milliseconds
     */
    private String startTime;

    /**
     * Bitrate, kb/s
     */
    private String bitrate;

    /**
     * Metadata map
     */
    private Map<String, String> metadata;

    /**
     * List of streams
     */
    private List<StreamInformation> streams;

    /**
     * Raw unparsed media information
     */
    private String rawInformation;

    public MediaInformation() {
        this.metadata = new HashMap<>();
        this.streams = new ArrayList<>();
    }

    public static MediaInformation from(final String rawCommandOutput) {
        final MediaInformation mediaInformation = new MediaInformation();

        if (rawCommandOutput != null) {
            final String[] split = rawCommandOutput.split("\n");
            boolean metadata = false;

            for (final String outputLine : split) {
                if (outputLine.startsWith("[")) {
                    metadata = false;
                    continue;
                }

                final String trimmedLine = outputLine.trim();

                if (trimmedLine.startsWith("Input")) {
                    metadata = false;
                    Pair<String, String> pair = parseInputBlock(trimmedLine);
                    mediaInformation.setFormat(pair.first);
                    mediaInformation.setPath(pair.second);
                } else if (trimmedLine.startsWith("Duration")) {
                    metadata = false;
                    System.out.println("Duration = " + trimmedLine);
                } else if (trimmedLine.startsWith("Metadata")) {
                    metadata = true;
                } else if (trimmedLine.startsWith("Stream mapping") || trimmedLine.startsWith("Press [q] to stop") || trimmedLine.startsWith("Output")) {
                    metadata = false;
                    break;
                } else if (trimmedLine.startsWith("Stream")) {
                    metadata = false;
                    System.out.println("Stream = " + trimmedLine);
                } else if (metadata) {
                    Pair<String, String> pair = parseMetadataBlock(trimmedLine);

                    if (pair.first != null && pair.second != null) {
                        mediaInformation.addMetadata(pair.first, pair.second);
                    }
                }
            }
        }

        return mediaInformation;
    }

    public static Pair<String, String> parseInputBlock(final String input) {
        String format = substring(input, ",", ", from", Collections.<String>emptyList());
        String path = substring(input, "\'", "\'", Collections.<String>emptyList());

        return new Pair<>(format, path);
    }

    public static Pair<String, String> parseDurationBlock(final String line) {
        String duration = substring(line, "Duration:", ",", Collections.singletonList("uration:"));
        String start = substring(line, "start:", ",", Collections.singletonList("tart:"));
        String bitrate = substring(line, "bitrate:", Arrays.asList("itrate:", "kb/s"));

        try {
            final Date calculated = DURATION_FORMAT.parse("00:00:00.040");

            System.out.println("Duration: " + (calculated.getTime() - REFERENCE_DURATION.getTime()));
        } catch (final ParseException e) {
            e.printStackTrace();
        }

        return new Pair<>(null, null);
    }

    public static Long parseDuration(final String duration) {
        if (duration == null || duration.equals("N/A")) {
            return null;
        }

        try {
            final Date calculated = DURATION_FORMAT.parse(duration);

            return (calculated.getTime() - REFERENCE_DURATION.getTime());
        } catch (final ParseException e) {
            return null;
        }
    }

    public static Long parseStartTime(final String startTime) {
        if (startTime == null || startTime.equals("N/A")) {
            return null;
        }

        try {
            final Date calculated = START_TIME_FORMAT.parse(startTime);

            return (calculated.getTime() - REFERENCE_START_TIME.getTime());
        } catch (final ParseException e) {
            return null;
        }
    }

    public static Pair<String, String> parseMetadataBlock(final String metadata) {
        String key = null;
        String value = null;

        if (metadata != null) {
            int index = metadata.indexOf(':');
            if (index > -1) {
                key = metadata.substring(0, index).trim();
                value = metadata.substring(index + 1).trim();
            }
        }

        return new Pair<>(key, value);
    }

    public static String substring(final String string, final String start, final String end, final List<String> ignoredTokens) {
        String extractedSubstring = null;

        if (string != null) {
            int formatStart = string.indexOf(start);
            if (formatStart > -1) {

                int formatEnd = string.indexOf(end, formatStart + 1);
                if (formatEnd > -1) {
                    extractedSubstring = string.substring(formatStart + 1, formatEnd);
                }
            }
        }

        if ((ignoredTokens != null) && (extractedSubstring != null)) {
            for (String token : ignoredTokens) {
                extractedSubstring = extractedSubstring.replaceAll(token, "");
            }
        }

        return (extractedSubstring == null) ? null : extractedSubstring.trim();
    }

    public static String substring(final String string, final String start, final List<String> ignoredTokens) {
        String extractedSubstring = null;

        if (string != null) {
            int formatStart = string.indexOf(start);
            if (formatStart > -1) {
                extractedSubstring = string.substring(formatStart + 1);
            }
        }

        if ((ignoredTokens != null) && (extractedSubstring != null)) {
            for (String token : ignoredTokens) {
                extractedSubstring = extractedSubstring.replaceAll(token, "");
            }
        }

        return (extractedSubstring == null) ? null : extractedSubstring.trim();
    }

    public String getFormat() {
        return format;
    }

    public void setFormat(String format) {
        this.format = format;
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public String getDuration() {
        return duration;
    }

    public void setDuration(String duration) {
        this.duration = duration;
    }

    public String getStartTime() {
        return startTime;
    }

    public void setStartTime(String startTime) {
        this.startTime = startTime;
    }

    public String getBitrate() {
        return bitrate;
    }

    public void setBitrate(String bitrate) {
        this.bitrate = bitrate;
    }

    public String getRawInformation() {
        return rawInformation;
    }

    public void setRawInformation(String rawInformation) {
        this.rawInformation = rawInformation;
    }

    public void addMetadata(String key, String value) {
        this.metadata.put(key, value);
    }

    public Set<Map.Entry<String, String>> getMetadataEntries() {
        return this.metadata.entrySet();
    }

    public void addStream(StreamInformation stream) {
        this.streams.add(stream);
    }

    public List<StreamInformation> getStreams() {
        return streams;
    }

}
