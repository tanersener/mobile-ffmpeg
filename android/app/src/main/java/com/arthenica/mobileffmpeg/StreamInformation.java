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

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class StreamInformation {

    /**
     * Stream index
     */
    private Long index;

    private String type;
    private String codec;
    private String fullCodec;
    private String format;
    private String fullFormat;

    private Long width;
    private Long height;

    private Long bitrate;
    private Long sampleRate;
    private String sampleFormat;
    private String channelLayout;

    /**
     * SAR
     */
    private String sampleAspectRatio;

    /**
     * DAR
     */
    private String displayAspectRatio;

    /**
     * fps
     */
    private String averageFrameRate;

    /**
     * tbr
     */
    private String realFrameRate;

    /**
     * tbn
     */
    private String timeBase;

    /**
     * tbc
     */
    private String codecTimeBase;

    /**
     * Metadata map
     */
    private final Map<String, String> metadata;

    public StreamInformation() {
        this.metadata = new HashMap<>();
    }

    public Long getIndex() {
        return index;
    }

    public void setIndex(Long index) {
        this.index = index;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getCodec() {
        return codec;
    }

    public void setCodec(String codec) {
        this.codec = codec;
    }

    public String getFullCodec() {
        return fullCodec;
    }

    public void setFullCodec(String fullCodec) {
        this.fullCodec = fullCodec;
    }

    public String getFormat() {
        return format;
    }

    public void setFormat(String format) {
        this.format = format;
    }

    public String getFullFormat() {
        return fullFormat;
    }

    public void setFullFormat(String fullFormat) {
        this.fullFormat = fullFormat;
    }

    public Long getWidth() {
        return width;
    }

    public void setWidth(Long width) {
        this.width = width;
    }

    public Long getHeight() {
        return height;
    }

    public void setHeight(Long height) {
        this.height = height;
    }

    public Long getBitrate() {
        return bitrate;
    }

    public void setBitrate(Long bitrate) {
        this.bitrate = bitrate;
    }

    public Long getSampleRate() {
        return sampleRate;
    }

    public void setSampleRate(Long sampleRate) {
        this.sampleRate = sampleRate;
    }

    public String getSampleFormat() {
        return sampleFormat;
    }

    public void setSampleFormat(String sampleFormat) {
        this.sampleFormat = sampleFormat;
    }

    public String getChannelLayout() {
        return channelLayout;
    }

    public void setChannelLayout(String channelLayout) {
        this.channelLayout = channelLayout;
    }

    public String getSampleAspectRatio() {
        return sampleAspectRatio;
    }

    public void setSampleAspectRatio(String sampleAspectRatio) {
        this.sampleAspectRatio = sampleAspectRatio;
    }

    public String getDisplayAspectRatio() {
        return displayAspectRatio;
    }

    public void setDisplayAspectRatio(String displayAspectRatio) {
        this.displayAspectRatio = displayAspectRatio;
    }

    public String getAverageFrameRate() {
        return averageFrameRate;
    }

    public void setAverageFrameRate(String averageFrameRate) {
        this.averageFrameRate = averageFrameRate;
    }

    public String getRealFrameRate() {
        return realFrameRate;
    }

    public void setRealFrameRate(String realFrameRate) {
        this.realFrameRate = realFrameRate;
    }

    public String getTimeBase() {
        return timeBase;
    }

    public void setTimeBase(String timeBase) {
        this.timeBase = timeBase;
    }

    public String getCodecTimeBase() {
        return codecTimeBase;
    }

    public void setCodecTimeBase(String codecTimeBase) {
        this.codecTimeBase = codecTimeBase;
    }

    public void addMetadata(String key, String value) {
        this.metadata.put(key, value);
    }

    public Set<Map.Entry<String, String>> getMetadataEntries() {
        return this.metadata.entrySet();
    }

}
