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

/**
 * Stream information class.
 *
 * @since 3.0
 */
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

    /**
     * Side data map
     */
    private final Map<String, String> sidedata;

    public StreamInformation() {
        this.metadata = new HashMap<>();
        this.sidedata = new HashMap<>();
    }

    /**
     * Returns stream index.
     *
     * @return stream index, starting from zero
     */
    public Long getIndex() {
        return index;
    }

    /**
     * Sets stream index.
     *
     * @param index stream index, starting from zero
     */
    public void setIndex(Long index) {
        this.index = index;
    }

    /**
     * Returns stream type.
     *
     * @return stream type; audio or video
     */
    public String getType() {
        return type;
    }

    /**
     * Sets stream type.
     *
     * @param type stream type; audio or video
     */
    public void setType(String type) {
        this.type = type;
    }

    /**
     * Returns stream codec.
     *
     * @return stream codec
     */
    public String getCodec() {
        return codec;
    }

    /**
     * Sets stream codec.
     *
     * @param codec stream codec
     */
    public void setCodec(String codec) {
        this.codec = codec;
    }

    /**
     * Returns full stream codec.
     *
     * @return stream codec with additional profile and mode information
     */
    public String getFullCodec() {
        return fullCodec;
    }

    /**
     * Sets full stream codec.
     *
     * @param fullCodec stream codec with additional profile and mode information
     */
    public void setFullCodec(String fullCodec) {
        this.fullCodec = fullCodec;
    }

    /**
     * Returns stream format.
     *
     * @return stream format
     */
    public String getFormat() {
        return format;
    }

    /**
     * Sets stream format.
     *
     * @param format stream format
     */
    public void setFormat(String format) {
        this.format = format;
    }

    /**
     * Returns full stream format.
     *
     * @return stream format with
     */
    public String getFullFormat() {
        return fullFormat;
    }

    /**
     * Sets full stream format.
     *
     * @param fullFormat stream format with
     */
    public void setFullFormat(String fullFormat) {
        this.fullFormat = fullFormat;
    }

    /**
     * Returns width.
     *
     * @return width in pixels
     */
    public Long getWidth() {
        return width;
    }

    /**
     * Sets width.
     *
     * @param width width in pixels
     */
    public void setWidth(Long width) {
        this.width = width;
    }

    /**
     * Returns height.
     *
     * @return height in pixels
     */
    public Long getHeight() {
        return height;
    }

    /**
     * Sets height.
     *
     * @param height height in pixels
     */
    public void setHeight(Long height) {
        this.height = height;
    }

    /**
     * Returns bitrate.
     *
     * @return bitrate in kb/s
     */
    public Long getBitrate() {
        return bitrate;
    }

    /**
     * Sets bitrate.
     *
     * @param bitrate bitrate in kb/s
     */
    public void setBitrate(Long bitrate) {
        this.bitrate = bitrate;
    }

    /**
     * Returns sample rate.
     *
     * @return sample rate in hz
     */
    public Long getSampleRate() {
        return sampleRate;
    }

    /**
     * Sets sample rate.
     *
     * @param sampleRate sample rate in hz
     */
    public void setSampleRate(Long sampleRate) {
        this.sampleRate = sampleRate;
    }

    /**
     * Returns sample format.
     *
     * @return sample format
     */
    public String getSampleFormat() {
        return sampleFormat;
    }

    /**
     * Sets sample format.
     *
     * @param sampleFormat sample format
     */
    public void setSampleFormat(String sampleFormat) {
        this.sampleFormat = sampleFormat;
    }

    /**
     * Returns channel layout.
     *
     * @return channel layout
     */
    public String getChannelLayout() {
        return channelLayout;
    }

    /**
     * Sets channel layout.
     *
     * @param channelLayout channel layout
     */
    public void setChannelLayout(String channelLayout) {
        this.channelLayout = channelLayout;
    }

    /**
     * Returns sample aspect ratio.
     *
     * @return sample aspect ratio
     */
    public String getSampleAspectRatio() {
        return sampleAspectRatio;
    }

    /**
     * Sets sample aspect ratio.
     *
     * @param sampleAspectRatio sample aspect ratio
     */
    public void setSampleAspectRatio(String sampleAspectRatio) {
        this.sampleAspectRatio = sampleAspectRatio;
    }

    /**
     * Returns display aspect ratio.
     *
     * @return display aspect ratio
     */
    public String getDisplayAspectRatio() {
        return displayAspectRatio;
    }

    /**
     * Sets display aspect ratio.
     *
     * @param displayAspectRatio display aspect ratio
     */
    public void setDisplayAspectRatio(String displayAspectRatio) {
        this.displayAspectRatio = displayAspectRatio;
    }

    /**
     * Returns display aspect ratio.
     *
     * @return average frame rate in fps
     */
    public String getAverageFrameRate() {
        return averageFrameRate;
    }

    /**
     * Sets average frame rate.
     *
     * @param averageFrameRate average frame rate in fps
     */
    public void setAverageFrameRate(String averageFrameRate) {
        this.averageFrameRate = averageFrameRate;
    }

    /**
     * Returns real frame rate.
     *
     * @return real frame rate in tbr
     */
    public String getRealFrameRate() {
        return realFrameRate;
    }

    /**
     * Sets real frame rate.
     *
     * @param realFrameRate real frame rate in tbr
     */
    public void setRealFrameRate(String realFrameRate) {
        this.realFrameRate = realFrameRate;
    }

    /**
     * Returns time base.
     *
     * @return time base in tbn
     */
    public String getTimeBase() {
        return timeBase;
    }

    /**
     * Sets time base.
     *
     * @param timeBase time base in tbn
     */
    public void setTimeBase(String timeBase) {
        this.timeBase = timeBase;
    }

    /**
     * Returns codec time base.
     *
     * @return codec time base in tbc
     */
    public String getCodecTimeBase() {
        return codecTimeBase;
    }

    /**
     * Sets codec time base.
     *
     * @param codecTimeBase codec time base in tbc
     */
    public void setCodecTimeBase(String codecTimeBase) {
        this.codecTimeBase = codecTimeBase;
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
     * Retrieves metadata value associated with this key.
     *
     * @param key metadata key
     * @return metadata value associated with this key
     */
    public String getMetadata(String key) {
        return this.metadata.get(key);
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
     * Adds side data.
     *
     * @param key side data key
     * @param value side data value
     */
    public void addSidedata(String key, String value) {
        this.sidedata.put(key, value);
    }

    /**
     * Retrieves side data value associated with this key.
     *
     * @param key side data key
     * @return side data value associated with this key
     */
    public String getSidedata(String key) {
        return this.sidedata.get(key);
    }

    /**
     * Returns all side data entries.
     *
     * @return set of site data entries
     */
    public Set<Map.Entry<String, String>> getSidedataEntries() {
        return this.sidedata.entrySet();
    }

}
