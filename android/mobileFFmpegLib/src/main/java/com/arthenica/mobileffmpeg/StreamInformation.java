/*
 * Copyright (c) 2018, 2020 Taner Sener
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

import org.json.JSONObject;

/**
 * Stream information class.
 *
 * @since 3.0
 */
public class StreamInformation {

    private static final String KEY_INDEX = "index";
    private static final String KEY_TYPE = "codec_type";
    private static final String KEY_CODEC = "codec_name";
    private static final String KEY_CODEC_LONG = "codec_long_name";
    private static final String KEY_FORMAT = "pix_fmt";
    private static final String KEY_WIDTH = "width";
    private static final String KEY_HEIGHT = "height";
    private static final String KEY_BIT_RATE = "bit_rate";
    private static final String KEY_SAMPLE_RATE = "sample_rate";
    private static final String KEY_SAMPLE_FORMAT = "sample_fmt";
    private static final String KEY_CHANNEL_LAYOUT = "channel_layout";
    private static final String KEY_SAMPLE_ASPECT_RATIO = "sample_aspect_ratio";
    private static final String KEY_DISPLAY_ASPECT_RATIO = "display_aspect_ratio";
    private static final String KEY_AVERAGE_FRAME_RATE = "avg_frame_rate";
    private static final String KEY_REAL_FRAME_RATE = "r_frame_rate";
    private static final String KEY_TIME_BASE = "time_base";
    private static final String KEY_CODEC_TIME_BASE = "codec_time_base";
    private static final String KEY_TAGS = "tags";

    /**
     * Stores all properties.
     */
    private final JSONObject jsonObject;

    public StreamInformation(final JSONObject jsonObject) {
        this.jsonObject = jsonObject;
    }

    /**
     * Returns stream index.
     *
     * @return stream index, starting from zero
     */
    public Long getIndex() {
        return getNumberProperty(KEY_INDEX);
    }

    /**
     * Returns stream type.
     *
     * @return stream type; audio or video
     */
    public String getType() {
        return getStringProperty(KEY_TYPE);
    }

    /**
     * Returns stream codec.
     *
     * @return stream codec
     */
    public String getCodec() {
        return getStringProperty(KEY_CODEC);
    }

    /**
     * Returns full stream codec.
     *
     * @return stream codec with additional profile and mode information
     */
    public String getFullCodec() {
        return getStringProperty(KEY_CODEC_LONG);
    }

    /**
     * Returns stream format.
     *
     * @return stream format
     */
    public String getFormat() {
        return getStringProperty(KEY_FORMAT);
    }

    /**
     * Returns width.
     *
     * @return width in pixels
     */
    public Long getWidth() {
        return getNumberProperty(KEY_WIDTH);
    }

    /**
     * Returns height.
     *
     * @return height in pixels
     */
    public Long getHeight() {
        return getNumberProperty(KEY_HEIGHT);
    }

    /**
     * Returns bitrate.
     *
     * @return bitrate in kb/s
     */
    public String getBitrate() {
        return getStringProperty(KEY_BIT_RATE);
    }

    /**
     * Returns sample rate.
     *
     * @return sample rate in hz
     */
    public String getSampleRate() {
        return getStringProperty(KEY_SAMPLE_RATE);
    }

    /**
     * Returns sample format.
     *
     * @return sample format
     */
    public String getSampleFormat() {
        return getStringProperty(KEY_SAMPLE_FORMAT);
    }

    /**
     * Returns channel layout.
     *
     * @return channel layout
     */
    public String getChannelLayout() {
        return getStringProperty(KEY_CHANNEL_LAYOUT);
    }

    /**
     * Returns sample aspect ratio.
     *
     * @return sample aspect ratio
     */
    public String getSampleAspectRatio() {
        return getStringProperty(KEY_SAMPLE_ASPECT_RATIO);
    }

    /**
     * Returns display aspect ratio.
     *
     * @return display aspect ratio
     */
    public String getDisplayAspectRatio() {
        return getStringProperty(KEY_DISPLAY_ASPECT_RATIO);
    }

    /**
     * Returns display aspect ratio.
     *
     * @return average frame rate in fps
     */
    public String getAverageFrameRate() {
        return getStringProperty(KEY_AVERAGE_FRAME_RATE);
    }

    /**
     * Returns real frame rate.
     *
     * @return real frame rate in tbr
     */
    public String getRealFrameRate() {
        return getStringProperty(KEY_REAL_FRAME_RATE);
    }

    /**
     * Returns time base.
     *
     * @return time base in tbn
     */
    public String getTimeBase() {
        return getStringProperty(KEY_TIME_BASE);
    }

    /**
     * Returns codec time base.
     *
     * @return codec time base in tbc
     */
    public String getCodecTimeBase() {
        return getStringProperty(KEY_CODEC_TIME_BASE);
    }

    /**
     * Returns all tags.
     *
     * @return tags dictionary
     */
    public JSONObject getTags() {
        return getProperties(KEY_TAGS);
    }

    /**
     * Returns the stream property associated with the key.
     *
     * @param key property key
     * @return stream property as string or null if the key is not found
     */
    public String getStringProperty(final String key) {
        JSONObject mediaProperties = getAllProperties();
        if (mediaProperties == null) {
            return null;
        }

        if (mediaProperties.has(key)) {
            return mediaProperties.optString(key);
        } else {
            return null;
        }
    }

    /**
     * Returns the stream property associated with the key.
     *
     * @param key property key
     * @return stream property as Long or null if the key is not found
     */
    public Long getNumberProperty(String key) {
        JSONObject mediaProperties = getAllProperties();
        if (mediaProperties == null) {
            return null;
        }

        if (mediaProperties.has(key)) {
            return mediaProperties.optLong(key);
        } else {
            return null;
        }
    }

    /**
     * Returns the stream properties associated with the key.
     *
     * @param key properties key
     * @return stream properties as a JSONObject or null if the key is not found
     */
    public JSONObject getProperties(String key) {
        JSONObject mediaProperties = getAllProperties();
        if (mediaProperties == null) {
            return null;
        }

        return mediaProperties.optJSONObject(key);
    }

    /**
     * Returns all stream properties defined.
     *
     * @return all stream properties as a JSONObject or null if no properties are defined
     */
    public JSONObject getAllProperties() {
        return jsonObject;
    }

}
