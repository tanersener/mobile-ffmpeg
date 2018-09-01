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

#include "ArchDetect.h"
#include "MobileFFmpegConfig.h"
#include "MobileFFmpeg.h"

@implementation ArchDetect

+ (void)initialize {
    [MobileFFmpegConfig class];
    [MobileFFmpeg class];
}

/**
 * Returns running architecture name.
 *
 * \return running architecture name as NSString
 */
+ (NSString*)getArch {
    NSMutableString *cpu = [[NSMutableString alloc] init];
    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);

    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);

    if (type == CPU_TYPE_X86_64) {
        [cpu appendString:@"x86_64"];

    } else if (type == CPU_TYPE_X86) {
        [cpu appendString:@"x86"];

    } else if (type == CPU_TYPE_ARM) {
        [cpu appendString:@"ARM"];

        switch(subtype) {
            case CPU_SUBTYPE_ARM_V6:
                [cpu appendString:@"V6"];
            break;
            case CPU_SUBTYPE_ARM_V7:
                [cpu appendString:@"V7"];
                break;
            case CPU_SUBTYPE_ARM_V8:
                [cpu appendString:@"V8"];
                break;
            default:
                [cpu appendString:@"unknown"];
                break;
        }
    }

    return cpu;
}

@end
