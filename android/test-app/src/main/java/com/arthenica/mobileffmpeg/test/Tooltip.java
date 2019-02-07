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
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg.test;

public interface Tooltip {

    // COMMAND TEST
    String COMMAND_TEST_TOOLTIP_TEXT = "Enter an FFmpeg command without 'ffmpeg' at the beginning and click one of the RUN buttons";

    // VIDEO TEST
    String VIDEO_TEST_TOOLTIP_TEXT = "Select a video codec and press ENCODE button";

    // HTTPS TEST
    String HTTPS_TEST_TOOLTIP_TEXT = "Enter the https url of a media file and click the button";

    // AUDIO TEST
    String AUDIO_TEST_TOOLTIP_TEXT = "Select an audio codec and press ENCODE button";

    // SUBTITLE TEST
    String SUBTITLE_TEST_TOOLTIP_TEXT = "Click the button to burn subtitles. Created video will play inside the frame below";

    // VID.STAB TEST
    String VIDSTAB_TEST_TOOLTIP_TEXT = "Click the button to stabilize video. Original video will play above and stabilized video will play below";

    // PIPE TEST
    String PIPE_TEST_TOOLTIP_TEXT = "Click the button to create a video using pipe redirection. Created video will play inside the frame below";

}
