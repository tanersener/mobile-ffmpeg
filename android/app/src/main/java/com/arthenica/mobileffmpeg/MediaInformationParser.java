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

import com.arthenica.mobileffmpeg.util.Pair;
import com.arthenica.mobileffmpeg.util.Trio;

import java.math.BigDecimal;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * Helper class for {@link MediaInformation}.
 *
 * @since 3.0
 */
public class MediaInformationParser {

    public static SimpleDateFormat DURATION_FORMAT;

    public static Date REFERENCE_DURATION;

    static {
        try {
            DURATION_FORMAT = new SimpleDateFormat("kk:mm:ss", Locale.getDefault());
            REFERENCE_DURATION = DURATION_FORMAT.parse("00:00:00");
        } catch (final ParseException e) {
            Log.i(Config.TAG, "Preparing duration reference failed.", e);
            DURATION_FORMAT = null;
            REFERENCE_DURATION = null;
        }
    }

    /**
     * Parses media information command output and builds a {@link MediaInformation} instance.
     *
     * @param rawCommandOutput media information command output
     * @return parsed instance of null if a parsing error occurs
     */
    public static MediaInformation from(final String rawCommandOutput) {
        final MediaInformation mediaInformation = new MediaInformation();

        if (rawCommandOutput != null) {
            final String[] split = rawCommandOutput.split("\n");
            boolean metadata = false;
            boolean sidedata = false;
            StreamInformation lastCreatedStream = null;
            final StringBuilder rawInformation = new StringBuilder();

            for (final String outputLine : split) {
                if (outputLine.startsWith("[")) {
                    metadata = false;
                    sidedata = false;
                    continue;
                }

                final String trimmedLine = outputLine.trim();

                if (trimmedLine.startsWith("Input")) {
                    metadata = false;
                    sidedata = false;
                    lastCreatedStream = null;
                    Pair<String, String> pair = parseInputBlock(trimmedLine);
                    mediaInformation.setFormat(pair.getFirst());
                    mediaInformation.setPath(pair.getSecond());
                } else if (trimmedLine.startsWith("Duration")) {
                    metadata = false;
                    sidedata = false;
                    lastCreatedStream = null;
                    Trio<Long, Long, Long> trio = parseDurationBlock(trimmedLine);
                    mediaInformation.setDuration(trio.getFirst());
                    mediaInformation.setStartTime(trio.getSecond());
                    mediaInformation.setBitrate(trio.getThird());
                } else if (trimmedLine.toLowerCase().startsWith("metadata")) {
                    sidedata = false;
                    metadata = true;
                } else if (trimmedLine.toLowerCase().startsWith("side data")) {
                    metadata = false;
                    sidedata = true;
                } else if (trimmedLine.startsWith("Stream mapping") || trimmedLine.startsWith("Press [q] to stop") || trimmedLine.startsWith("Output")) {
                    break;
                } else if (trimmedLine.startsWith("Stream")) {
                    metadata = false;
                    sidedata = false;
                    lastCreatedStream = MediaInformationParser.parseStreamBlock(trimmedLine);
                    mediaInformation.addStream(lastCreatedStream);
                } else if (metadata) {
                    Pair<String, String> pair = parseMetadataBlock(trimmedLine);

                    if (pair.getFirst() != null && pair.getSecond() != null) {
                        if (lastCreatedStream != null) {
                            lastCreatedStream.addMetadata(pair.getFirst(), pair.getSecond());
                        } else {
                            mediaInformation.addMetadata(pair.getFirst(), pair.getSecond());
                        }
                    }
                } else if (sidedata) {
                    Pair<String, String> pair = parseMetadataBlock(trimmedLine);

                    if (pair.getFirst() != null && pair.getSecond() != null) {
                        if (lastCreatedStream != null) {
                            lastCreatedStream.addSidedata(pair.getFirst(), pair.getSecond());
                        }
                    }
                }

                rawInformation.append(outputLine);
                rawInformation.append("\n");
            }

            mediaInformation.setRawInformation(rawInformation.toString());
        }

        return mediaInformation;
    }

    static Pair<String, String> parseInputBlock(final String input) {
        String format = substring(input, ",", ", from", Collections.<String>emptyList());
        String path = substring(input, "\'", "\'", Collections.<String>emptyList());

        return new Pair<>(format, path);
    }

    static Trio<Long, Long, Long> parseDurationBlock(final String line) {
        Long duration = parseDuration(substring(line, "Duration:", ",", Collections.singletonList("uration:")));
        Long start = parseStartTime(substring(line, "start:", ",", Collections.singletonList("tart:")));
        Long bitrate = toLongObject(substring(line, "bitrate:", Arrays.asList("itrate:", "kb/s")));

        return new Trio<>(duration, start, bitrate);
    }

    static Pair<String, String> parseMetadataBlock(final String metadata) {
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

    static StreamInformation parseStreamBlock(final String input) {
        final StreamInformation streamInformation = new StreamInformation();

        if (input != null) {
            streamInformation.setIndex(parseStreamIndex(input));

            int typeBlockStartIndex = index(input, ":", 0, 2);
            if (typeBlockStartIndex > -1 && (typeBlockStartIndex < input.length())) {
                String[] parts = input.substring(typeBlockStartIndex + 1).split(",");
                String typePart = safeGet(parts, 0);

                final String type = parseStreamType(typePart);
                streamInformation.setType(type);
                streamInformation.setCodec(parseStreamCodec(typePart));
                streamInformation.setFullCodec(parseStreamFullCodec(typePart));

                String part2 = safeGet(parts, 1);
                String part3 = safeGet(parts, 2);
                String part4 = safeGet(parts, 3);
                String part5 = safeGet(parts, 4);
                if ("video".equals(type)) {
                    int lastUsedPart = 1;

                    if (part2 != null) {
                        int pStart = count(part2, "(");
                        int pEnd = count(part2, ")");

                        while (pStart != pEnd) {
                            lastUsedPart++;
                            String newPart = safeGet(parts, lastUsedPart);
                            if (newPart == null) {
                                break;
                            }
                            part2 = String.format("%s,%s", part2, newPart);
                            pStart = count(part2, "(");
                            pEnd = count(part2, ")");
                        }

                        streamInformation.setFullFormat(part2.toLowerCase(Locale.getDefault()).trim());
                        streamInformation.setFormat(part2.replaceAll("\\(.*\\)", "").toLowerCase(Locale.getDefault()).trim());
                    }

                    lastUsedPart++;
                    String videoDimensionPart = safeGet(parts, lastUsedPart);
                    if (videoDimensionPart != null) {
                        String videoLayout = videoDimensionPart.toLowerCase(Locale.getDefault()).trim();
                        Pair<Long, Long> dimensions = parseVideoDimensions(videoLayout);
                        streamInformation.setWidth(dimensions.getFirst());
                        streamInformation.setHeight(dimensions.getSecond());
                        streamInformation.setSampleAspectRatio(parseVideoStreamSampleAspectRatio(videoLayout));
                        streamInformation.setDisplayAspectRatio(parseVideoStreamDisplayAspectRatio(videoLayout));
                    }

                    for (int i = lastUsedPart + 1; i < parts.length; i++) {
                        String part = parts[i].replaceAll("\\(.*\\)", "").toLowerCase(Locale.getDefault());

                        if (part.contains("kb/s")) {
                            streamInformation.setBitrate(toLongObject(part.replaceAll("kb/s", "").trim()));
                        } else if (part.contains("fps")) {
                            streamInformation.setAverageFrameRate(part.replaceAll("fps", "").trim());
                        } else if (part.contains("tbr")) {
                            streamInformation.setRealFrameRate(part.replaceAll("tbr", "").trim());
                        } else if (part.contains("tbn")) {
                            streamInformation.setTimeBase(part.replaceAll("tbn", "").trim());
                        } else if (part.contains("tbc")) {
                            streamInformation.setCodecTimeBase(part.replaceAll("tbc", "").trim());
                        }
                    }
                } else if ("audio".equals(type)) {
                    if (part2 != null) {
                        streamInformation.setSampleRate(parseAudioStreamSampleRate(part2));
                    }
                    if (part3 != null) {
                        streamInformation.setChannelLayout(part3.toLowerCase(Locale.getDefault()).trim());
                    }
                    if (part4 != null) {
                        streamInformation.setSampleFormat(part4.toLowerCase(Locale.getDefault()).trim());
                    }
                    if (part5 != null) {
                        streamInformation.setBitrate(toLongObject(part5.toLowerCase(Locale.getDefault()).replaceAll("\\(.*\\)", "").replaceAll("kb/s", "").trim()));
                    }
                } else if ("data".equals(type)) {
                    if (part2 != null) {
                        streamInformation.setBitrate(toLongObject(part2.toLowerCase(Locale.getDefault()).replaceAll("\\(.*\\)", "").replaceAll("kb/s", "").trim()));
                    }
                }
            }
        }

        return streamInformation;
    }

    static Pair<Long, Long> parseVideoDimensions(final String input) {
        Long width = null;
        Long height = null;

        if (input != null) {
            final String[] dimensions = input.toLowerCase(Locale.getDefault()).replaceAll("\\[.*\\]", "").trim().split("x");
            width = toLongObject(safeGet(dimensions, 0));
            height = toLongObject(safeGet(dimensions, 1));
        }

        return new Pair<>(width, height);
    }

    static String parseVideoStreamSampleAspectRatio(final String input) {
        if (input != null) {
            String[] parts = input.replaceAll("\\[", "").replaceAll("\\]", "").split(" ");
            for (int i = 0; i < parts.length; i++) {
                if (parts[i].toLowerCase(Locale.getDefault()).equals("sar")) {
                    return safeGet(parts, i + 1);
                }
            }
        }

        return null;
    }

    static String parseVideoStreamDisplayAspectRatio(final String input) {
        if (input != null) {
            String[] parts = input.replaceAll("\\[", "").replaceAll("\\]", "").split(" ");
            for (int i = 0; i < parts.length; i++) {
                if (parts[i].toLowerCase(Locale.getDefault()).equals("dar")) {
                    return safeGet(parts, i + 1);
                }
            }
        }

        return null;
    }

    static Long parseAudioStreamSampleRate(final String input) {
        if (input != null) {
            boolean khz = false;
            boolean mhz = false;

            String lowerCase = input.toLowerCase(Locale.getDefault());
            if (lowerCase.contains("khz")) {
                khz = true;
            }
            if (lowerCase.contains("mhz")) {
                mhz = true;
            }

            String sampleRate = lowerCase
                    .replaceAll("khz", "")
                    .replaceAll("mhz", "")
                    .replaceAll("hz", "")
                    .trim();

            if (khz) {
                return 1000 * toLong(sampleRate);
            } else if (mhz) {
                return 1000000 * toLong(sampleRate);
            } else {
                return toLong(sampleRate);
            }
        }

        return null;
    }

    static String parseStreamType(final String input) {
        if (input != null) {
            if (input.toLowerCase(Locale.getDefault()).contains("audio:")) {
                return "audio";
            } else if (input.toLowerCase(Locale.getDefault()).contains("video:")) {
                return "video";
            } else if (input.toLowerCase(Locale.getDefault()).contains("data:")) {
                return "data";
            }
        }

        return null;
    }

    static String parseStreamCodec(final String input) {
        if (input != null) {
            return input.toLowerCase(Locale.getDefault())
                    .replaceAll("\\(.*\\)", "")
                    .replaceAll("video:", "")
                    .replaceAll("audio:", "")
                    .replaceAll("data:", "")
                    .trim();
        }

        return null;
    }

    static String parseStreamFullCodec(final String input) {
        if (input != null) {
            return input.toLowerCase(Locale.getDefault())
                    .replaceAll("video:", "")
                    .replaceAll("audio:", "")
                    .replaceAll("data:", "")
                    .trim();
        }

        return null;
    }

    static Long parseStreamIndex(final String input) {
        String substring = substring(input, "Stream #0:", ":", Collections.singletonList("tream #0"));
        if (substring != null) {

            // DISCARD PARANTHESIS
            return toLongObject(substring
                    .replace(":", "")
                    .replaceAll("\\(.*\\)", ""));
        }

        return null;
    }

    static Long parseDuration(final String duration) {
        if (duration == null || duration.equals("N/A")) {
            return null;
        }

        try {
            final Date calculated = DURATION_FORMAT.parse(duration);

            long secondsPartInMilliseconds = calculated.getTime() - REFERENCE_DURATION.getTime();

            int index = duration.indexOf('.');
            if (index > -1) {
                Long centiSeconds = toLong(duration.substring(index + 1));
                secondsPartInMilliseconds += 10 * centiSeconds;
            }

            return secondsPartInMilliseconds;

        } catch (final ParseException e) {
            Log.d(Config.TAG, String.format("Parsing duration: %s failed.", duration), e);
            return null;
        }
    }

    static Long parseStartTime(final String startTime) {
        if (startTime == null || startTime.equals("N/A")) {
            return null;
        }

        try {
            BigDecimal bigDecimal = new BigDecimal(startTime);
            bigDecimal = bigDecimal.setScale(3, BigDecimal.ROUND_CEILING).multiply(new BigDecimal(1000));
            return bigDecimal.longValue();
        } catch (NumberFormatException e) {
            Log.d(Config.TAG, String.format("Parsing startTime: %s failed.", startTime), e);
            return null;
        }
    }

    public static String substring(final String string, final String start, final String end, final List<String> ignoredTokens) {
        String extractedSubstring = null;

        if (string != null) {
            int formatStart = string.indexOf(start);
            if (formatStart > -1) {

                int formatEnd = string.indexOf(end, formatStart + start.length());
                if (formatEnd > -1) {
                    extractedSubstring = string.substring(formatStart + start.length(), formatEnd);
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

    public static int index(final String string, String substring, int startIndex, int n) {
        int count = 1;

        while (count <= n) {
            startIndex = string.indexOf(substring, startIndex + substring.length());
            count++;
        }

        return startIndex;
    }

    public static int count(final String string, String substring) {
        int count = 0;
        int index = 0;

        do {
            index = string.indexOf(substring, index);
            if (index >= 0) {
                count++;
                index = index + substring.length();
            }
        } while (index >= 0);

        return count;
    }

    private static <K> K safeGet(final K[] array, final int index) {
        if (array == null) {
            return null;
        }

        try {
            final int size = array.length;
            if (size > index) {
                return array[index];
            } else {
                return null;
            }
        } catch (final ArrayIndexOutOfBoundsException e) {
            return null;
        }
    }

    private static long toLong(final String value) {
        try {
            return Long.parseLong(value);
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    static Long toLongObject(final String value) {
        try {
            return Long.parseLong(value);
        } catch (NumberFormatException e) {
            return null;
        }
    }

}
