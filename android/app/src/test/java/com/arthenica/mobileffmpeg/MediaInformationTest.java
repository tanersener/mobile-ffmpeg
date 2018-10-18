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

import junit.framework.Assert;

import org.junit.Test;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class MediaInformationTest {

    @Test
    public void parseDuration() {
        parseDuration(null, null);
        parseDuration("", null);
        parseDuration("N/A", null);
        parseDuration("00:10:34.53", 634530L);
    }

    @Test
    public void parseStartTime() {
        parseStartTime(null, null);
        parseStartTime("", null);
        parseStartTime("0.000000", 0L);
        parseStartTime("324.000000", 324000L);
        parseStartTime("-4.000000", -4000L);
        parseStartTime("10.003000", 10003L);
    }

    @Test
    public void parseDurationBlock() {
        // parseDuration("  Duration: 00:00:00.04, start: 0.000000, bitrate: 391187 kb/s", null, null);

        final SimpleDateFormat dateFormat = new SimpleDateFormat("ss.SSSSSS");

        try {
            final Date reference = dateFormat.parse("0.00");
            final Date calculated = dateFormat.parse("-0.005000");

            System.out.println("Duration: " + (calculated.getTime() - reference.getTime()));
        } catch (final ParseException e) {
            e.printStackTrace();
        }

    }

    @Test
    public void parseInputBlock() {
        parseInputBlock("Input #0,", null, null);
        parseInputBlock("Input #0, ogg, from 'trailer_400p.ogg':", "ogg", "trailer_400p.ogg");
        parseInputBlock("Input #0, mp3, from 'cold_stone_-_planet_x.mp3':", "mp3", "cold_stone_-_planet_x.mp3");
        parseInputBlock("Input #0, image2, from '/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg':", "image2", "/data/user/0/com.arthenica.mobileffmpeg.test/cache/colosseum.jpg");
        parseInputBlock("Input #0, gif, from 'advanced_zoom_in_and_pan_with_fade_in_out.gif':", "gif", "advanced_zoom_in_and_pan_with_fade_in_out.gif");
        parseInputBlock("Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'transition_rotate.mp4':", "mov,mp4,m4a,3gp,3g2,mj2", "transition_rotate.mp4");
        parseInputBlock("Input #0, png_pipe, from 'https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png':", "png_pipe", "https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png");
    }

    @Test
    public void parseMetadataBlock() {
        parseMetadataBlock("      ENCODER:", "ENCODER", "");
        parseMetadataBlock("      ENCODER:ffmpeg2theora 0.19", "ENCODER", "ffmpeg2theora 0.19");
        parseMetadataBlock("      ENCODER         : ffmpeg2theora 0.19", "ENCODER", "ffmpeg2theora 0.19");
        parseMetadataBlock("      creation_time   : 2013-12-16T17:50:04.000000Z", "creation_time", "2013-12-16T17:50:04.000000Z");
        parseMetadataBlock("      handler_name    : GPAC ISO Audio Handler", "handler_name", "GPAC ISO Audio Handler");
        parseMetadataBlock("    comment         : Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net", "comment", "Creative Commons Attribution 3.0 - http://bbb3d.renderfarming.net");
        parseMetadataBlock("    minor_version   : 1", "minor_version", "1");
        parseMetadataBlock("    encoder         : Lavf58.12.100", "encoder", "Lavf58.12.100");
        parseMetadataBlock("    title           : Planet X", "title", "Planet X");
        parseMetadataBlock("    compatible_brands: isomiso2avc1mp41", "compatible_brands", "isomiso2avc1mp41");
    }

    public void parseDuration(String value, Long expected) {
        Long duration = MediaInformation.parseDuration(value);

        Assert.assertEquals(expected, duration);
    }

    public void parseStartTime(String value, Long expected) {
        Long duration = MediaInformation.parseStartTime(value);

        Assert.assertEquals(expected, duration);
    }

    public void parseDurationBlock(String value, String first, String second) {
        Pair<String, String> pair = MediaInformation.parseDurationBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertEquals(first, pair.first);
        Assert.assertEquals(second, pair.second);
    }

    public void parseInputBlock(String value, String first, String second) {
        Pair<String, String> pair = MediaInformation.parseInputBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertEquals(first, pair.first);
        Assert.assertEquals(second, pair.second);
    }

    public void parseMetadataBlock(String value, String first, String second) {
        Pair<String, String> pair = MediaInformation.parseMetadataBlock(value);

        Assert.assertNotNull(pair);
        Assert.assertNotNull(pair.first);
        Assert.assertNotNull(pair.second);
        Assert.assertEquals(first, pair.first);
        Assert.assertEquals(second, pair.second);
    }

}
