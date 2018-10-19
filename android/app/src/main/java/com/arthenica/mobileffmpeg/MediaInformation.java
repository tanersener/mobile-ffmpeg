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

    public Long getDuration() {
        return duration;
    }

    public void setDuration(Long duration) {
        this.duration = duration;
    }

    public Long getStartTime() {
        return startTime;
    }

    public void setStartTime(Long startTime) {
        this.startTime = startTime;
    }

    public Long getBitrate() {
        return bitrate;
    }

    public void setBitrate(Long bitrate) {
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
