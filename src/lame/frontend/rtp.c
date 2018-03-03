/*
 *      rtp socket communication functions
 *
 *      initially contributed by Felix von Leitner
 *
 *      Copyright (c) 2000 Mark Taylor
 *                    2010 Robert Hegemann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: rtp.c,v 1.24 2011/05/07 16:05:17 rbrito Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

struct rtpbits {
    int     sequence:16;     /* sequence number: random */
    int     pt:7;            /* payload type: 14 for MPEG audio */
    int     m:1;             /* marker: 0 */
    int     cc:4;            /* number of CSRC identifiers: 0 */
    int     x:1;             /* number of extension headers: 0 */
    int     p:1;             /* is there padding appended: 0 */
    int     v:2;             /* version: 2 */
};

struct rtpheader {           /* in network byte order */
    struct rtpbits b;
    int     timestamp;       /* start: random */
    int     ssrc;            /* random */
    int     iAudioHeader;    /* =0?! */
};


#if !defined( _WIN32 ) && !defined(__MINGW32__)

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdarg.h>
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#ifdef __int8_t_defined
#undef uint8_t
#undef uint16_t
#undef uint32_t
#undef uint64_t
#endif
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "rtp.h"
#include "console.h"

typedef int SOCKET;

struct rtpheader RTPheader;
SOCKET  rtpsocket;


/* create a sender socket. */
int
rtp_socket(char const *address, unsigned int port, unsigned int TTL)
{
    int     iRet, iLoop = 1;
    struct sockaddr_in sin;
    unsigned char cTtl = TTL;
    char    cLoop = 0;
    unsigned int tempaddr;

    int     iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket < 0) {
        error_printf("socket() failed.\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    tempaddr = inet_addr(address);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = tempaddr;

    iRet = setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, &iLoop, sizeof(int));
    if (iRet < 0) {
        error_printf("setsockopt SO_REUSEADDR failed\n");
        return 1;
    }

    if ((ntohl(tempaddr) >> 28) == 0xe) {
        /* only set multicast parameters for multicast destination IPs */
        iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_TTL, &cTtl, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_TTL failed.  multicast in kernel?\n");
            return 1;
        }

        cLoop = 1;      /* !? */
        iRet = setsockopt(iSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &cLoop, sizeof(char));
        if (iRet < 0) {
            error_printf("setsockopt IP_MULTICAST_LOOP failed.  multicast in kernel?\n");
            return 1;
        }
    }
    iRet = connect(iSocket, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    if (iRet < 0) {
        error_printf("connect IP_MULTICAST_LOOP failed.  multicast in kernel?\n");
        return 1;
    }

    rtpsocket = iSocket;

    return 0;
}


static void
rtp_initialization_extra(void)
{
}

static void
rtp_close_extra(void)
{
}

#else

#include <Winsock2.h>
#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 3
#endif
#include <stdio.h>
#include <stdarg.h>

#include "rtp.h"
#include "console.h"


struct rtpheader RTPheader;
SOCKET  rtpsocket;

static char *
last_error_message(int err_code)
{
    char   *msg;
    void   *p_msg_buf;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   (void *) 0,
                   (DWORD) err_code,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) & p_msg_buf, 0, NULL);
    msg = strdup(p_msg_buf);
    LocalFree(p_msg_buf);
    return msg;
}

static int
print_socket_error(int error)
{
    char   *err_txt = last_error_message(error);
    error_printf("error %d\n%s\n", error, err_txt);
    free(err_txt);
    return error;
}

static int
on_socket_error(SOCKET s)
{
    int     error = WSAGetLastError();
    print_socket_error(error);
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    return error;
}

/* create a sender socket. */
int
rtp_socket(char const *address, unsigned int port, unsigned int TTL)
{
    char const True = 1;
    char const *c = "";
    int     error;
    UINT    ip;
    PHOSTENT host;
    SOCKET  s;
    SOCKADDR_IN source, dest;

    source.sin_family = AF_INET;
    source.sin_addr.s_addr = htonl(INADDR_ANY);
    source.sin_port = htons(0);

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(address);

    if (!strcmp(address, "255.255.255.255")) {
    }
    else if (dest.sin_addr.s_addr == INADDR_NONE) {
        host = gethostbyname(address);

        if (host) {
            dest.sin_addr = *(PIN_ADDR) host->h_addr;
        }
        else {
            error_printf("Unknown host %s\r\n", address);
            return 1;
        }
    }

    dest.sin_port = htons((u_short) port);

    ip = ntohl(dest.sin_addr.s_addr);

    if (IN_CLASSA(ip))
        c = "class A";
    if (IN_CLASSB(ip))
        c = "class B";
    if (IN_CLASSC(ip))
        c = "class C";
    if (IN_CLASSD(ip))
        c = "class D";
    if (ip == INADDR_LOOPBACK)
        c = "loopback";
    if (ip == INADDR_BROADCAST)
        c = "broadcast";

    s = socket(AF_INET, SOCK_DGRAM, PF_UNSPEC);
    if (s == INVALID_SOCKET) {
        error_printf("socket () ");
        return on_socket_error(s);
    }
    error = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(True));
    error = bind(s, (struct sockaddr *) &source, sizeof(source));
    if (error == SOCKET_ERROR) {
        error_printf("bind () ");
        return on_socket_error(s);
    }
    if (ip == INADDR_BROADCAST) {
        error_printf("broadcast %s:%u %s\r\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), c);
        error = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &True, sizeof(True));
        if (error == SOCKET_ERROR) {
            error_printf("setsockopt (%u, SOL_SOCKET, SO_BROADCAST, ...) ", s);
            return on_socket_error(s);
        }
    }
    if (IN_CLASSD(ip)) {
        error_printf("multicast %s:%u %s\r\n", inet_ntoa(dest.sin_addr), ntohs(dest.sin_port), c);
        error = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) &TTL, sizeof(TTL));
        if (error == SOCKET_ERROR) {
            error_printf("setsockopt (%u, IPPROTO_IP, IP_MULTICAST_TTL, ...) ", s);
            return on_socket_error(s);
        }
    }
    error = connect(s, (PSOCKADDR) & dest, sizeof(SOCKADDR_IN));
    if (error == SOCKET_ERROR) {
        error_printf("connect: ");
        return on_socket_error(s);
    }
    rtpsocket = s;
    return 0;
}

static void
rtp_initialization_extra(void)
{
    WSADATA wsaData;
    int     rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        print_socket_error(rc);
    }
}

static void
rtp_close_extra(void)
{
    WSACleanup();
}

#endif


static int
rtp_send(unsigned char const *data, int len)
{
    SOCKET  s = rtpsocket;
    struct rtpheader *foo = &RTPheader;
    char   *buffer = malloc(len + sizeof(struct rtpheader));
    int    *cast = (int *) foo;
    int    *outcast = (int *) buffer;
    int     count, size;

    outcast[0] = htonl(cast[0]);
    outcast[1] = htonl(cast[1]);
    outcast[2] = htonl(cast[2]);
    outcast[3] = htonl(cast[3]);
    memmove(buffer + sizeof(struct rtpheader), data, len);
    size = len + sizeof(*foo);
    count = send(s, buffer, size, 0);
    free(buffer);

    return count != size;
}

void
rtp_output(unsigned char const *mp3buffer, int mp3size)
{
    rtp_send(mp3buffer, mp3size);
    RTPheader.timestamp += 5;
    RTPheader.b.sequence++;
}

void
rtp_initialization(void)
{
    struct rtpheader *foo = &RTPheader;
    foo->b.v = 2;
    foo->b.p = 0;
    foo->b.x = 0;
    foo->b.cc = 0;
    foo->b.m = 0;
    foo->b.pt = 14;     /* MPEG Audio */
    foo->b.sequence = rand() & 65535;
    foo->timestamp = rand();
    foo->ssrc = rand();
    foo->iAudioHeader = 0;
    rtp_initialization_extra();
}

void
rtp_deinitialization(void)
{
    rtp_close_extra();
}
