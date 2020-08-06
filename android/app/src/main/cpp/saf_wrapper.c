/*
 * Copyright (c) 2020 Taner Sener
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

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "libavformat/avformat.h"
#include "libavutil/avstring.h"

#include "mobileffmpeg.h"
#include "saf_wrapper.h"

// in these wrappers, we call the original functions, so we remove the shadow defines
#undef avio_closep
#undef avformat_close_input
#undef avio_open
#undef avio_open2
#undef avformat_open_input

static int fd_read_packet(void* opaque, uint8_t* buf, int buf_size) {
    int fd = (int)opaque;
    return read(fd, buf, buf_size);
}

static int fd_write_packet(void* opaque, uint8_t* buf, int buf_size) {
    int fd = (int)opaque;
    return write(fd, buf, buf_size);
}

static int64_t fd_seek(void *opaque, int64_t offset, int whence) {
    int fd = (int)opaque;

    if (fd < 0) {
        return AVERROR(EINVAL);
    }
    LOGD("fd_seek fd=%d %lld %d\n", fd, (long long)offset, whence);

    int64_t ret;
    if (whence == AVSEEK_SIZE) {
        struct stat st;
        ret = fstat(fd, &st);
        return ret < 0 ? AVERROR(errno) : (S_ISFIFO(st.st_mode) ? 0 : st.st_size);
    }

    ret = lseek(fd, offset, whence);

    return ret < 0 ? AVERROR(errno) : ret;
}

__thread char link_name_buf[1024];
static const char *get_link_name(int fd) {
        char path[1024];

        sprintf(path, "/proc/self/fd/%d", fd);
        int nbytes = readlink(path, link_name_buf, sizeof(link_name_buf));
        link_name_buf[nbytes] = '\0';
        return link_name_buf;
}

static AVIOContext *maybe_get_fd_avio_context(const char *filename, int write_flag) {
    union {int fd; void* opaque;} fdunion;
    fdunion.fd = -1;
    const char *fd_ptr = NULL;
    if (av_strstart(filename, "saf:", &fd_ptr)) {
        char *final;
        fdunion.fd = strtol(fd_ptr, &final, 10);
        if (fd_ptr == final) {/* No digits found */
            fdunion.fd = -1;
        }
    }

    if (fdunion.fd >= 0) {
        LOGD("recovered fd=%d for %s. Size is %lld -> %s\n", fdunion.fd, write_flag ? "write" : "read", (long long)fd_seek(fdunion.opaque, 0, AVSEEK_SIZE), get_link_name(fdunion.fd));
        return avio_alloc_context(av_malloc(4096), 4096, write_flag, fdunion.opaque, fd_read_packet, write_flag ? fd_write_packet : NULL, fd_seek);
    }
    return NULL;
}

static void release_fd_avio_contextp(AVIOContext **ctx) {
    if (fd_seek((*ctx)->opaque, 0, AVSEEK_SIZE) >= 0) {
        LOGD("release_fd_avio_context %p->%d\n", *ctx, (int)(*ctx)->opaque);
        close((int)(*ctx)->opaque);
        *ctx = NULL;
    }
}

int android_avformat_open_input(AVFormatContext **ps, const char *filename,
                        ff_const59 AVInputFormat *fmt, AVDictionary **options) {
    if (!(*ps) && !(*ps = avformat_alloc_context()))
        return AVERROR(ENOMEM);

    (*ps)->pb = maybe_get_fd_avio_context(filename, 0);

    return avformat_open_input(ps, filename, fmt, options);
}

int android_avio_open2(AVIOContext **s, const char *filename, int flags,
               const AVIOInterruptCB *int_cb, AVDictionary **options) {
    if ((*s = maybe_get_fd_avio_context(filename, (flags & AVIO_FLAG_WRITE) != 0 ? 1 : 0))) {
        // for saf: pseudo-protocol, int_cb is silently ignored
        return 0;
    }
    return avio_open2(s, filename, flags, int_cb, options);
}

int android_avio_open(AVIOContext **s, const char *url, int flags) {
    return android_avio_open2(s, url, flags, NULL, NULL);
}

int android_avio_closep(AVIOContext **s) {
    release_fd_avio_contextp(s);
    return avio_closep(s);
}

void android_avformat_close_input(AVFormatContext **ps) {
    if (*ps && (*ps)->pb) {
        release_fd_avio_contextp(&(*ps)->pb);
    }
    avformat_close_input(ps);
}