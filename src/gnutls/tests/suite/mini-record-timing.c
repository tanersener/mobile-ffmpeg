/*
 * Copyright (C) 2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)

int main()
{
	exit(77);
}

#else

#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

//#define USE_RDTSC
//#define TEST_ETM

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifdef USE_RDTSC
#include <x86intrin.h>
#endif

#ifdef DEBUG
static void server_log_func(int level, const char *str)
{
	fprintf(stderr, "server|<%d>| %s", level, str);
}

static void client_log_func(int level, const char *str)
{
	fprintf(stderr, "client|<%d>| %s", level, str);
}
#endif



#ifndef _POSIX_TIMERS
#error need posix timers
#endif

#define CLOCK_TO_USE CLOCK_MONOTONIC
//#define CLOCK_TO_USE CLOCK_MONOTONIC_RAW
//#define CLOCK_TO_USE CLOCK_PROCESS_CPUTIME_ID

/* This program tests the robustness of record
 * decoding.
 */
#define MAX_PER_POINT (8*1024)
#define WARM_UP (2)
#define MAX_BUF 1024

struct point_st {
	unsigned char byte1;
	unsigned char byte2;
	unsigned midx;
	unsigned taken;
	unsigned long *smeasurements;
};

struct test_st {
	struct point_st *points;
	unsigned int npoints;
	const char *desc;
	const char *file;
	const char *name;
	unsigned text_size;
	uint8_t fill;
};

struct point_st *prev_point_ptr = NULL;
unsigned int point_idx = 0;
static gnutls_session_t cli_session = NULL;

static ssize_t
push(gnutls_transport_ptr_t tr, const void *_data, size_t len)
{
	int fd = (long int) tr;

	return send(fd, _data, len, 0);
}

static ssize_t
push_crippled(gnutls_transport_ptr_t tr, const void *_data, size_t len)
{
	int fd = (long int) tr;
	unsigned char *data = (void *) _data;
	struct point_st *p;
	unsigned p_size;
	struct test_st *test = gnutls_session_get_ptr(cli_session);

	p_size = test->npoints;

	if (point_idx >= p_size)
		abort();

	p = &test->points[point_idx];


	if (test->fill != 0xff) {
		/* original lucky13 attack */
		memmove(&data[len - 32], data + 5, 32);
		data[len - 17] ^= p->byte1;
		data[len - 18] ^= p->byte2;
	} else {
		/* a revised attack which depends on chosen-plaintext */
		assert(len>512);
		memmove(data+len-256-32, data+5, 256+32);
		data[len - 257] ^= p->byte1;
	}

	return send(fd, data, len, 0);
}


#ifndef USE_RDTSC
static unsigned long timespec_sub_ns(struct timespec *a,
				     struct timespec *b)
{
	return (a->tv_sec - b->tv_sec) * 1000 * 1000 * 1000 + a->tv_nsec -
		b->tv_nsec;
}
#endif

static void
client(int fd, const char *prio, unsigned int text_size,
       struct test_st *test)
{
	int ret;
	char buffer[MAX_BUF + 1];
	char *text;
	gnutls_psk_client_credentials_t pskcred;
	gnutls_session_t session;
	static unsigned long measurement;
	const gnutls_datum_t key = { (void *) "DEADBEEF", 8 };
	const char *err;
	unsigned j;

	gnutls_global_init();

	text = malloc(text_size);
	assert(text != NULL);

	setpriority(PRIO_PROCESS, getpid(), -15);

	memset(text, test->fill, text_size);

#ifdef DEBUG
	gnutls_global_set_log_function(client_log_func);
	gnutls_global_set_log_level(6);
#endif

	gnutls_psk_allocate_client_credentials(&pskcred);
	gnutls_psk_set_client_credentials(pskcred, "test", &key,
					  GNUTLS_PSK_KEY_HEX);

	/* Initialize TLS session
	 */
	assert(gnutls_init(&session, GNUTLS_CLIENT) >= 0);
	gnutls_session_set_ptr(session, test);
	cli_session = session;

	/* Use default priorities */
	if ((ret = gnutls_priority_set_direct(session, prio, &err)) < 0) {
		fprintf(stderr, "Error in priority string %s: %s\n",
			gnutls_strerror(ret), err);
		exit(1);
	}

	/* put the anonymous credentials to the current session
	 */
	gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred);
	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fprintf(stderr, "client: Handshake failed: %s\n", gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_protocol_get_version(session);
	if (ret < GNUTLS_TLS1_1) {
		fprintf(stderr,
			"client: Handshake didn't negotiate TLS 1.1 (or later)\n");
		exit(1);
	}

	gnutls_transport_set_push_function(session, push_crippled);

 restart:
	do {
		ret = gnutls_record_send(session, text, sizeof(text));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	/* measure peer's processing time */

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret < 0
		 && (ret == GNUTLS_E_AGAIN
		     || ret == GNUTLS_E_INTERRUPTED));

	if (ret > 0) {
		struct point_st *point_ptr = NULL;

		assert(ret == sizeof(measurement));

		point_ptr = &test->points[point_idx];
		point_ptr->taken++;

		if (point_idx == 0) {
			printf("%s: measurement: %u / %d\r", test->name, (unsigned)point_ptr->taken+1, MAX_PER_POINT+WARM_UP);
		}

		if (point_idx == 0 && point_ptr->midx+1 >= MAX_PER_POINT) {
			goto finish;
		}

		if (point_ptr->taken >= WARM_UP) {
			memcpy(&measurement, buffer, sizeof(measurement));
			if (point_ptr->midx < MAX_PER_POINT) {
				point_ptr->smeasurements[point_ptr->midx] =
				    measurement;
				point_ptr->midx++;
				point_idx++;

				if (point_idx >= test->npoints)
					point_idx = 0;
			}
		}

		goto restart;
	} else {
		abort();
	}

finish:
	fprintf(stderr, "\ntest completed\n");

	gnutls_transport_set_push_function(session, push);

	gnutls_bye(session, GNUTLS_SHUT_WR);

	{
		unsigned i;
		FILE *fp = NULL;

		fp = fopen(test->file, "w");
		assert(fp != NULL);

		fprintf(fp, "Delta,");
		for (j = 0; j < MAX_PER_POINT; j++) {
			fprintf(fp, "measurement-%u%s", j, j!=(MAX_PER_POINT-1)?",":"");
		}
		fprintf(fp, "\n");
		for (i = 0; i < test->npoints; i++) {
			fprintf(fp, "%u,", (unsigned)test->points[i].byte1);
			for (j = 0; j < MAX_PER_POINT; j++) {
				fprintf(fp, "%u%s",
					(unsigned) test->points[i].smeasurements[j],
					(j!=MAX_PER_POINT-1)?",":"");

			}
			fprintf(fp, "\n");
		}
		printf("\n");

		fclose(fp);
	}

	if (test->desc)
		fprintf(stderr, "Description: %s\n", test->desc);

	close(fd);

	gnutls_deinit(session);

	gnutls_psk_free_client_credentials(pskcred);

	gnutls_global_deinit();
	free(text);
}

static int
pskfunc(gnutls_session_t session, const char *username,
	gnutls_datum_t * key)
{
	key->data = gnutls_malloc(4);
	key->data[0] = 0xDE;
	key->data[1] = 0xAD;
	key->data[2] = 0xBE;
	key->data[3] = 0xEF;
	key->size = 4;
	return 0;
}

static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_psk_server_credentials_t server_pskcred;
	const char *err;
#ifndef USE_RDTSC
	struct timespec start, stop;
#else
	uint64_t c1, c2;
	unsigned int i1;
#endif
	static unsigned long measurement;

	setpriority(PRIO_PROCESS, getpid(), -15);

	gnutls_global_init();
	memset(buffer, 0, sizeof(buffer));

#ifdef DEBUG
	gnutls_global_set_log_function(server_log_func);
	gnutls_global_set_log_level(6);
#endif
	assert(gnutls_psk_allocate_server_credentials(&server_pskcred)>=0);
	gnutls_psk_set_server_credentials_function(server_pskcred,
						   pskfunc);
	assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	if ((ret = gnutls_priority_set_direct(session, prio, &err)) < 0) {
		fprintf(stderr, "Error in priority string %s: %s\n",
			gnutls_strerror(ret), err);
		exit(1);
	}

	gnutls_credentials_set(session, GNUTLS_CRD_PSK, server_pskcred);
	gnutls_transport_set_int(session, fd);

	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
#ifdef GNUTLS_E_PREMATURE_TERMINATION
		if (ret != GNUTLS_E_PREMATURE_TERMINATION
		    && ret != GNUTLS_E_UNEXPECTED_PACKET_LENGTH)
#else
		if (ret != GNUTLS_E_UNEXPECTED_PACKET_LENGTH)
#endif
		{
			fprintf(stderr,
				"server: Handshake has failed (%s)\n\n",
				gnutls_strerror(ret));
			exit(1);
		}
		goto finish;
	}
#ifdef TEST_ETM
	assert(gnutls_session_etm_status(session)!=0);
#else
	assert(gnutls_session_etm_status(session)==0);
#endif

 restart:
	do {
		ret = recv(fd, buffer, 1, MSG_PEEK);
	} while (ret == -1 && errno == EAGAIN);

#ifdef USE_RDTSC
	c1 = __rdtscp(&i1);
#else
	clock_gettime(CLOCK_TO_USE, &start);
#endif

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

#ifdef USE_RDTSC
	c2 = __rdtscp(&i1);
	measurement = c2 - c1;
#else
	clock_gettime(CLOCK_TO_USE, &stop);
	measurement = timespec_sub_ns(&stop, &start);
#endif

	if (ret == GNUTLS_E_DECRYPTION_FAILED) {
		gnutls_session_force_valid(session);
		do {
			ret =
			    gnutls_record_send(session, &measurement,
					       sizeof(measurement));
			/* GNUTLS_AL_FATAL, GNUTLS_A_BAD_RECORD_MAC); */
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

		if (ret >= 0) {
			goto restart;
		}
	} else if (ret > 0) {
		goto restart;
	}

	assert(ret <= 0);

	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

      finish:
	close(fd);
	gnutls_deinit(session);

	gnutls_psk_free_server_credentials(server_pskcred);

	gnutls_global_deinit();
}

static void start(const char *prio, unsigned int text_size,
		  struct test_st *p)
{
	int fd[2];
	int ret;
	pid_t child;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	if (ret < 0) {
		perror("socketpair");
		exit(1);
	}

	child = fork();
	if (child < 0) {
		perror("fork");
		fprintf(stderr, "fork");
		exit(1);
	}

	if (child != 0) {
		/* parent */
		close(fd[1]);
		server(fd[0], prio);
		kill(child, SIGTERM);
	} else if (child == 0) {
		close(fd[0]);
		client(fd[1], prio, text_size, p);
		exit(0);
	}
}

static void ch_handler(int sig)
{
	int status;
	wait(&status);
	if (WEXITSTATUS(status) != 0 ||
	    (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV)) {
		if (WIFSIGNALED(status))
			fprintf(stderr, "Child died with sigsegv\n");
		else
			fprintf(stderr, "Child died with status %d\n",
				WEXITSTATUS(status));
	}
	return;
}

#define NPOINTS 256
static struct point_st all_points[NPOINTS];
static struct point_st all_points_one[NPOINTS];

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA1.
 */
static struct test_st test_sha1 = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 18 * 16,
	.name = "sha1",
	.file = "out-sha1.txt",
	.fill = 0x00,
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA256.
 */
static struct test_st test_sha256 = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 17 * 16,
	.name = "sha256",
	.file = "out-sha256.txt",
	.fill = 0x00,
	.desc = NULL
};

static struct test_st test_sha256_new = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 1024 * 16,
	.name = "sha256-new",
	.file = "out-sha256-new.txt",
	.fill = 0xff,
	.desc = NULL
};

static struct test_st test_sha384 = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 33 * 16,
	.name = "sha384",
	.file = "out-sha384.txt",
	.fill = 0x00,
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA1, on a short message.
 */
static struct test_st test_sha1_short = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 16 * 2,
	.name = "sha1-short",
	.file = "out-sha1-short.txt",
	.fill = 0x00,
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA256.
 */
static struct test_st test_sha256_short = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 16 * 2,
	.name = "sha256-short",
	.file = "out-sha256-short.txt",
	.fill = 0x00,
	.desc = NULL
};

static struct test_st test_sha384_short = {
	.points = all_points,
	.npoints = NPOINTS,
	.text_size = 16 * 2,
	.name = "sha384-short",
	.file = "out-sha384-short.txt",
	.fill = 0x00,
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA1, on a short message and
 * the byte before the last is set to one.
 * (i.e. we want to see whether the padding
 *  [1,1] shows up in the measurements)
 */
static struct test_st test_sha1_one = {
	.points = all_points_one,
	.npoints = NPOINTS,
	.text_size = 16 * 2,
	.name = "sha1-one",
	.file = "out-sha1-one.txt",
	.fill = 0x00,
	.desc = NULL
};

int main(int argc, char **argv)
{
	unsigned int i;
	struct test_st *test;
	const char *hash;
	char prio[512];
	struct timespec res;

	assert(clock_getres(CLOCK_TO_USE, &res) >= 0);
	assert(res.tv_nsec < 100);

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	if (argc > 1) {
		if (strcmp(argv[1], "sha1") == 0) {
			test = &test_sha1;
			hash = "SHA1";
		} else if (strcmp(argv[1], "sha1-short") == 0) {
			test = &test_sha1_short;
			hash = "SHA1";
		} else if (strcmp(argv[1], "sha256-short") == 0) {
			test = &test_sha256_short;
			hash = "SHA256";
		} else if (strcmp(argv[1], "sha256-new") == 0) {
			test = &test_sha256_new;
			hash = "SHA256";
		} else if (strcmp(argv[1], "sha256") == 0) {
			test = &test_sha256;
			hash = "SHA256";
		} else if (strcmp(argv[1], "sha384-short") == 0) {
			test = &test_sha384_short;
			hash = "SHA384";
		} else if (strcmp(argv[1], "sha384") == 0) {
			test = &test_sha384;
			hash = "SHA384";
		} else if (strcmp(argv[1], "sha1-one") == 0) {
			test = &test_sha1_one;
			hash = "SHA1";
		} else {
			fprintf(stderr, "Unknown test: %s\n", argv[1]);
			exit(1);
		}
	} else {
		fprintf(stderr,
			"Please specify the test, sha1, sha1-one, sha256, sha1-short, sha256-short\n");
		exit(1);
	}

	memset(&all_points, 0, sizeof(all_points));
	for (i = 0; i < 256; i++) {
		all_points[i].byte1 = i;
		all_points[i].byte2 = 0;
		all_points[i].smeasurements =
		    malloc(MAX_PER_POINT *
			   sizeof(all_points[i].smeasurements[0]));
	}

	memset(&all_points_one, 0, sizeof(all_points_one));
	for (i = 0; i < 256; i++) {
		all_points_one[i].byte1 = i;
		all_points_one[i].byte2 = 1;
		all_points_one[i].smeasurements =
		    all_points[i].smeasurements;
	}


	remove(test->file);
	snprintf(prio, sizeof(prio),
#ifdef TEST_ETM
		 "NONE:+COMP-NULL:+AES-128-CBC:+AES-256-CBC:+%s:+PSK:+VERS-TLS1.2:+VERS-TLS1.1:+SIGN-ALL:+CURVE-ALL",
#else
		 "NONE:+COMP-NULL:+AES-128-CBC:+AES-256-CBC:+%s:+PSK:%%NO_ETM:+VERS-TLS1.2:+VERS-TLS1.1:+SIGN-ALL:+CURVE-ALL",
#endif
		 hash);

	printf("\nAES-%s (calculating different padding timings)\n", hash);
	start(prio, test->text_size, test);

	signal(SIGCHLD, SIG_IGN);

	return 0;
}

#endif				/* _WIN32 */
