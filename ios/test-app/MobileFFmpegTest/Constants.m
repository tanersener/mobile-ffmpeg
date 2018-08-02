//
// Constants.m
//
// Copyright (c) 2018 Taner Sener
//
// This file is part of MobileFFmpeg.
//
// MobileFFmpeg is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MobileFFmpeg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
//

#import "Constants.h"

// COMMAND TEST
NSString *const COMMAND_TEST_TOOLTIP_TEXT = @"Enter an FFmpeg command without 'ffmpeg' at the beginning and click one of the RUN buttons";
NSTimeInterval const COMMAND_TEST_TOOLTIP_DURATION = 5.0;

// VIDEO TEST
NSString *const VIDEO_TEST_TOOLTIP_TEXT = @"Select a video codec and press ENCODE button";
NSTimeInterval const VIDEO_TEST_TOOLTIP_DURATION = 5.0;

// HTTPS TEST
NSString *const HTTPS_TEST_DEFAULT_URL = @"https://download.blender.org/peach/trailer/trailer_400p.ogg";
NSString *const HTTPS_TEST_TOOLTIP_TEXT = @"Enter the https url of a media file and click the button";
NSTimeInterval const HTTPS_TEST_TOOLTIP_DURATION = 5.0;
