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
    private Integer index;

    private String type;
    private String codec;
    private String format;

    private String width;
    private String height;

    /**
     * Sample Aspect Ratio
     */
    private String sar;

    /**
     * Display Aspect Ratio
     */
    private String dar;

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

    public Integer getIndex() {
        return index;
    }

    public void setIndex(Integer index) {
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

    public String getFormat() {
        return format;
    }

    public void setFormat(String format) {
        this.format = format;
    }

    public String getWidth() {
        return width;
    }

    public void setWidth(String width) {
        this.width = width;
    }

    public String getHeight() {
        return height;
    }

    public void setHeight(String height) {
        this.height = height;
    }

    public String getSar() {
        return sar;
    }

    public void setSar(String sar) {
        this.sar = sar;
    }

    public String getDar() {
        return dar;
    }

    public void setDar(String dar) {
        this.dar = dar;
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
