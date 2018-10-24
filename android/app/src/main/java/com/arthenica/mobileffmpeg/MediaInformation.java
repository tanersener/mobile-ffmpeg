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
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Media information class.
 *
 * @since 3.0
 */
public class MediaInformation {

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
    private Long duration;

    /**
     * Start time, in milliseconds
     */
    private Long startTime;

    /**
     * Bitrate, kb/s
     */
    private Long bitrate;

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

    /**
     * Returns format.
     *
     * @return media format
     */
    public String getFormat() {
        return format;
    }

    /**
     * Sets media format.
     *
     * @param format media format
     */
    public void setFormat(String format) {
        this.format = format;
    }

    /**
     * Returns path.
     *
     * @return media path
     */
    public String getPath() {
        return path;
    }

    /**
     * Sets path.
     *
     * @param path media path
     */
    public void setPath(String path) {
        this.path = path;
    }

    /**
     * Returns duration.
     *
     * @return media duration in milliseconds
     */
    public Long getDuration() {
        return duration;
    }

    /**
     * Sets duration.
     *
     * @param duration media duration in milliseconds
     */
    public void setDuration(Long duration) {
        this.duration = duration;
    }

    /**
     * Returns start time.
     *
     * @return media start time in milliseconds
     */
    public Long getStartTime() {
        return startTime;
    }

    /**
     * Sets start time.
     *
     * @param startTime media start time in milliseconds
     */
    public void setStartTime(Long startTime) {
        this.startTime = startTime;
    }

    /**
     * Returns bitrate.
     *
     * @return media bitrate in kb/s
     */
    public Long getBitrate() {
        return bitrate;
    }

    /**
     * Sets bitrate.
     *
     * @param bitrate media bitrate in kb/s
     */
    public void setBitrate(Long bitrate) {
        this.bitrate = bitrate;
    }

    /**
     * Returns unparsed media information.
     *
     * @return unparsed media information data
     */
    public String getRawInformation() {
        return rawInformation;
    }

    /**
     * Sets unparsed media information.
     *
     * @param rawInformation unparsed media information data
     */
    public void setRawInformation(String rawInformation) {
        this.rawInformation = rawInformation;
    }

    /**
     * Adds metadata.
     *
     * @param key metadata key
     * @param value metadata value
     */
    public void addMetadata(String key, String value) {
        this.metadata.put(key, value);
    }

    /**
     * Returns all metadata entries.
     *
     * @return set of metadata entries
     */
    public Set<Map.Entry<String, String>> getMetadataEntries() {
        return this.metadata.entrySet();
    }

    /**
     * Adds new stream.
     *
     * @param stream new stream information
     */
    public void addStream(StreamInformation stream) {
        this.streams.add(stream);
    }

    /**
     * Returns all streams.
     *
     * @return list of streams
     */
    public List<StreamInformation> getStreams() {
        return streams;
    }

}
