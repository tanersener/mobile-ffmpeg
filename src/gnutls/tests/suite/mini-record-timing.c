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
#include <errno.h>
#include "../utils.h"

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

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

#undef REHANDSHAKE

/* This program tests the robustness of record
 * decoding.
 */

static unsigned char server_cert_pem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBeTCCASWgAwIBAgIBBzANBgkqhkiG9w0BAQsFADAPMQ0wCwYDVQQDEwROb25l\n"
    "MCIYDzIwMTMwMTE5MTA0MDAwWhgPMjA0MDA2MDUxMDQwMDBaMA8xDTALBgNVBAMT\n"
    "BE5vbmUwWTANBgkqhkiG9w0BAQEFAANIADBFAj4Bh52/b3FNXDdICg1Obqu9ivW+\n"
    "PGJ89mNsX3O9S/aclnx5Ozw9MC1UJuZ2UEHl27YVmm4xG/y3nKUNevZjKwIDAQAB\n"
    "o2swaTAMBgNVHRMBAf8EAjAAMBQGA1UdEQQNMAuCCWxvY2FsaG9zdDATBgNVHSUE\n"
    "DDAKBggrBgEFBQcDATAPBgNVHQ8BAf8EBQMDB6AAMB0GA1UdDgQWBBRhEgmVCi6c\n"
    "hhRQvMzfEXqLKTRxcTANBgkqhkiG9w0BAQsFAAM/AADMi31wr0Tp2SJUCuQjFVCb\n"
    "JDleomTayOWVS/afCyAUxYjqFfUFSZ8sYN3zAgnXt5DYO3VclIlax4n6iXOg\n"
    "-----END CERTIFICATE-----\n";

const gnutls_datum_t server_cert = { server_cert_pem,
	sizeof(server_cert_pem)
};

static unsigned char server_key_pem[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIBLAIBAAI+AYedv29xTVw3SAoNTm6rvYr1vjxifPZjbF9zvUv2nJZ8eTs8PTAt\n"
    "VCbmdlBB5du2FZpuMRv8t5ylDXr2YysCAwEAAQI9EPt8Q77sFeWn0BfHoPD9pTsG\n"
    "5uN2e9DP8Eu6l8K4AcOuEsEkqZzvxgqZPA68pw8BZ5xKINMFdRPHmrX/cQIfHsdq\n"
    "aMDYR/moqgj8MbupqOr/48iorTk/D//2lgAMnwIfDLk3UWGvPiv6fNTlEnTgVn6o\n"
    "TdL0mvpkixebQ5RR9QIfHDjkRGtXph+xXUBh50RZXE8nFfl/WV7diVE+DOq8pwIf\n"
    "BxdOwjdsAH1oLBxG0sN6qBoM2NrCYoE8edydNsu55QIfEWsrlJnO/t0GzHy7qWdV\n"
    "zi9JMPu9MTDhOGmqPQO7Xw==\n" "-----END RSA PRIVATE KEY-----\n";


const gnutls_datum_t server_key = { server_key_pem,
	sizeof(server_key_pem)
};


/* A very basic TLS client, with anonymous authentication.
 */


#define MAX_PER_POINT (684*1024)
#define MAX_MEASUREMENTS(np) (MAX_PER_POINT*(np))
#define MAX_BUF 1024

struct point_st {
	unsigned char byte1;
	unsigned char byte2;
	unsigned midx;
	unsigned long *measurements;
	unsigned long *smeasurements;
};

struct test_st {
	struct point_st *points;
	unsigned int npoints;
	const char *desc;
	const char *file;
	const char *name;
	unsigned text_size;
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

	p = &test->points[point_idx];
	p_size = test->npoints;

	memcpy(&data[len - 32], data + 5, 32);

/*fprintf(stderr, "sending: %d: %d\n", (unsigned)p->byte1, (int)len);*/
	data[len - 17] ^= p->byte1;
	data[len - 18] ^= p->byte2;

	prev_point_ptr = p;
	point_idx++;
	if (point_idx >= p_size)
		point_idx = 0;

	return send(fd, data, len, 0);
}


static unsigned long timespec_sub_ns(struct timespec *a,
				     struct timespec *b)
{
	return (a->tv_sec * 1000 * 1000 * 1000 + a->tv_nsec -
		(b->tv_sec * 1000 * 1000 * 1000 + b->tv_nsec));
}

static
double calc_avg(unsigned long *diffs, unsigned int diffs_size)
{
	double avg = 0;
	unsigned int i;
	unsigned int start = diffs_size / 20;
	unsigned int stop = diffs_size - diffs_size / 20;

	for (i = start; i < stop; i++)
		avg += diffs[i];

	avg /= (stop - start);

	return avg;
}

static int compar(const void *_a, const void *_b)
{
	unsigned long a, b;

	a = *((unsigned long *) _a);
	b = *((unsigned long *) _b);

	if (a < b)
		return -1;
	else if (a == b)
		return 0;
	else
		return 1;
}

static
double calc_median(unsigned long *diffs, unsigned int diffs_size)
{
	double med;

	if (diffs_size % 2 == 1)
		med = diffs[diffs_size / 2];
	else {
		med = diffs[diffs_size / 2] + diffs[(diffs_size - 1) / 2];
		med /= 2;
	}

	return med;
}

#if 0
static
unsigned long calc_min(unsigned long *diffs, unsigned int diffs_size)
{
	unsigned long min = 0, i;
	unsigned int start = diffs_size / 20;
	unsigned int stop = diffs_size - diffs_size / 20;


	for (i = start; i < stop; i++) {
		if (min == 0)
			min = diffs[i];
		else if (diffs[i] < min)
			min = diffs[i];
	}
	return min;
}

static
double calc_var(unsigned long *diffs, unsigned int diffs_size, double avg)
{
	double sum = 0, d;
	unsigned int i;
	unsigned int start = diffs_size / 20;
	unsigned int stop = diffs_size - diffs_size / 20;

	for (i = start; i < stop; i++) {
		d = ((double) diffs[i] - avg);
		d *= d;

		sum += d;
	}
	sum /= diffs_size - 1;

	return sum;
}
#endif

static void
client(int fd, const char *prio, unsigned int text_size,
       struct test_st *test)
{
	int ret;
	char buffer[MAX_BUF + 1];
	char text[text_size];
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t session;
	struct timespec start, stop;
	static unsigned long taken = 0;
	static unsigned long measurement;
	const char *err;

	global_init();

	setpriority(PRIO_PROCESS, getpid(), -15);

	memset(text, 0, text_size);

#ifdef DEBUG
	gnutls_global_set_log_function(client_log_func);
	gnutls_global_set_log_level(6);
#endif

	gnutls_certificate_allocate_credentials(&x509_cred);

#ifdef REHANDSHAKE
      restart:
#endif

	/* Initialize TLS session
	 */
	gnutls_init(&session, GNUTLS_CLIENT);
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
	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	gnutls_transport_set_int(session, fd);

	/* Perform the TLS handshake
	 */
	do {
		ret = gnutls_handshake(session);
	}
	while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

	if (ret < 0) {
		fprintf(stderr, "client: Handshake failed\n");
		gnutls_perror(ret);
		exit(1);
	}

	ret = gnutls_protocol_get_version(session);
	if (ret < GNUTLS_TLS1_1) {
		fprintf(stderr,
			"client: Handshake didn't negotiate TLS 1.1 (or later)\n");
		exit(1);
	}

	gnutls_transport_set_push_function(session, push_crippled);

#ifndef REHANDSHAKE
      restart:
#endif
	do {
		ret = gnutls_record_send(session, text, sizeof(text));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);
	/* measure peer's processing time */
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

#define TLS_RECV
#ifdef TLS_RECV
	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret < 0
		 && (ret == GNUTLS_E_AGAIN
		     || ret == GNUTLS_E_INTERRUPTED));
#else
	do {
		ret = recv(fd, buffer, sizeof(buffer), 0);
	} while (ret == -1 && errno == EAGAIN);
#endif

	if (taken < MAX_MEASUREMENTS(test->npoints) && ret > 0) {
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
		taken++;
		measurement = timespec_sub_ns(&stop, &start);
		prev_point_ptr->measurements[prev_point_ptr->midx] =
		    measurement;

/*fprintf(stderr, "(%u,%u): %lu\n", (unsigned) prev_point_ptr->byte1,
 (unsigned) prev_point_ptr->byte2, measurements[taken]);*/
		memcpy(&measurement, buffer, sizeof(measurement));
		prev_point_ptr->smeasurements[prev_point_ptr->midx] =
		    measurement;
		prev_point_ptr->midx++;

		/* read server's measurement */

#ifdef REHANDSHAKE
		gnutls_deinit(session);
#endif
		goto restart;
	}
#ifndef TLS_RECV
	else if (ret < 0) {
		fprintf(stderr, "Error in recv()\n");
		exit(1);
	}
#endif

	gnutls_transport_set_push_function(session, push);

	gnutls_bye(session, GNUTLS_SHUT_WR);

	{
		double avg2, med, savg, smed;
		unsigned i;
		FILE *fp = NULL;

		if (test->file)
			fp = fopen(test->file, "w");

		if (fp)		/* point, avg, median */
			fprintf(fp,
				"Delta,TimeAvg,TimeMedian,ServerAvg,ServerMedian\n");

		for (i = 0; i < test->npoints; i++) {
			qsort(test->points[i].measurements,
			      test->points[i].midx,
			      sizeof(test->points[i].measurements[0]),
			      compar);

			qsort(test->points[i].smeasurements,
			      test->points[i].midx,
			      sizeof(test->points[i].smeasurements[0]),
			      compar);

			avg2 =
			    calc_avg(test->points[i].measurements,
				     test->points[i].midx);
			/*var = calc_var( test->points[i].measurements, test->points[i].midx, avg2); */
			med =
			    calc_median(test->points[i].measurements,
					test->points[i].midx);

			savg =
			    calc_avg(test->points[i].smeasurements,
				     test->points[i].midx);
			/*var = calc_var( test->points[i].measurements, test->points[i].midx, avg2); */
			smed =
			    calc_median(test->points[i].smeasurements,
					test->points[i].midx);
			/*min = calc_min( test->points[i].measurements, test->points[i].midx); */

			if (fp)	/* point, avg, median */
				fprintf(fp, "%u,%.2lf,%.2lf,%.2lf,%.2lf\n",
					(unsigned) test->points[i].byte1,
					avg2, med, savg, smed);

			/*printf("(%u) Avg: %.3f nanosec, Median: %.3f, Variance: %.3f\n", (unsigned)test->points[i].byte1, 
			   avg2, med, var); */
		}

		if (fp)
			fclose(fp);
	}

	if (test->desc)
		fprintf(stderr, "Description: %s\n", test->desc);

	close(fd);

	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();
}



static void server(int fd, const char *prio)
{
	int ret;
	char buffer[MAX_BUF + 1];
	gnutls_session_t session;
	gnutls_certificate_credentials_t x509_cred;
	const char *err;
	struct timespec start, stop;
	static unsigned long measurement;

	setpriority(PRIO_PROCESS, getpid(), -15);

	/* this must be called once in the program
	 */
	global_init();
	memset(buffer, 0, sizeof(buffer));

#ifdef DEBUG
	gnutls_global_set_log_function(server_log_func);
	gnutls_global_set_log_level(6);
#endif

	gnutls_certificate_allocate_credentials(&x509_cred);
	ret =
	    gnutls_certificate_set_x509_key_mem(x509_cred, &server_cert,
						&server_key,
						GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "Could not set certificate\n");
		return;
	}
#ifdef REHANDSHAKE
      restart:
#endif
	gnutls_init(&session, GNUTLS_SERVER);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	if ((ret = gnutls_priority_set_direct(session, prio, &err)) < 0) {
		fprintf(stderr, "Error in priority string %s: %s\n",
			gnutls_strerror(ret), err);
		return;
	}

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);
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
		}
		goto finish;
	}
#ifndef REHANDSHAKE
      restart:
#endif

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	do {
		ret = gnutls_record_recv(session, buffer, sizeof(buffer));
	} while (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);

	if (ret == GNUTLS_E_DECRYPTION_FAILED) {
		gnutls_session_force_valid(session);
		measurement = timespec_sub_ns(&stop, &start);
		do {
			ret =
			    gnutls_record_send(session, &measurement,
					       sizeof(measurement));
			/* GNUTLS_AL_FATAL, GNUTLS_A_BAD_RECORD_MAC); */
		} while (ret == GNUTLS_E_AGAIN
			 || ret == GNUTLS_E_INTERRUPTED);

#ifdef REHANDSHAKE
		gnutls_deinit(session);
#endif
		if (ret >= 0)
			goto restart;
	} else if (ret < 0)
		fprintf(stderr, "err: %s\n", gnutls_strerror(ret));


	/* do not wait for the peer to close the connection.
	 */
	gnutls_bye(session, GNUTLS_SHUT_WR);

      finish:
	close(fd);
	gnutls_deinit(session);

	gnutls_certificate_free_credentials(x509_cred);

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

static struct point_st all_points[256];
static struct point_st all_points_one[256];


#define NPOINTS(points) (sizeof(points)/sizeof(points[0]))

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA1.
 */
static struct test_st test_sha1 = {
	.points = all_points,
	.npoints = NPOINTS(all_points),
	.text_size = 18 * 16,
	.name = "sha1",
	.file = "out-sha1.txt",
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA256.
 */
static struct test_st test_sha256 = {
	.points = all_points,
	.npoints = NPOINTS(all_points),
	.text_size = 17 * 16,
	.name = "sha256",
	.file = "out-sha256.txt",
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA1, on a short message.
 */
static struct test_st test_sha1_short = {
	.points = all_points,
	.npoints = NPOINTS(all_points),
	.text_size = 16 * 2,
	.name = "sha1-short",
	.file = "out-sha1-short.txt",
	.desc = NULL
};

/* Test that outputs a graph of the timings
 * when manipulating the last record byte (pad)
 * for AES-SHA256.
 */
static struct test_st test_sha256_short = {
	.points = all_points,
	.npoints = NPOINTS(all_points),
	.text_size = 16 * 2,
	.name = "sha256-short",
	.file = "out-sha256-short.txt",
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
	.npoints = NPOINTS(all_points_one),
	.text_size = 16 * 2,
	.name = "sha1-one",
	.file = "out-sha1-one.txt",
	.desc = NULL
};

int main(int argc, char **argv)
{
	unsigned int i;
	struct test_st *test;
	const char *hash;
	char prio[512];

	signal(SIGCHLD, ch_handler);
	signal(SIGPIPE, SIG_IGN);

	if (argc > 1) {
		if (strcmp(argv[1], "sha1") == 0) {
			test = &test_sha1;
			hash = "SHA1";
		} else if (strncmp(argv[1], "sha2", 4) == 0) {
			test = &test_sha256;
			hash = "SHA256";
		} else if (strcmp(argv[1], "sha1-short") == 0) {
			test = &test_sha1_short;
			hash = "SHA1";
		} else if (strcmp(argv[1], "sha256-short") == 0) {
			test = &test_sha256_short;
			hash = "SHA256";
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
		all_points[i].measurements =
		    malloc(MAX_PER_POINT *
			   sizeof(all_points[i].measurements[0]));
		all_points[i].smeasurements =
		    malloc(MAX_PER_POINT *
			   sizeof(all_points[i].measurements[0]));
	}

	memset(&all_points_one, 0, sizeof(all_points_one));
	for (i = 0; i < 256; i++) {
		all_points_one[i].byte1 = i;
		all_points_one[i].byte2 = 1;
		all_points_one[i].measurements =
		    all_points[i].measurements;
		all_points_one[i].smeasurements =
		    all_points[i].smeasurements;
	}


	remove(test->file);
	snprintf(prio, sizeof(prio),
		 "NONE:+COMP-NULL:+AES-128-CBC:+%s:+RSA:%%COMPAT:+VERS-TLS1.2:+VERS-TLS1.1",
		 hash);

	printf("\nAES-%s (calculating different padding timings)\n", hash);
	start(prio, test->text_size, test);

	signal(SIGCHLD, SIG_IGN);

#ifdef PDF
	snprintf(prio, sizeof(prio),
		 "R -e 'pdf(file=\"%s-timings-avg.pdf\");z=read.csv(\"%s\");"
		 "plot(z$Delta,z$TimeAvg,xlab=\"Delta\",ylab=\"Average timings (ns)\");"
		 "dev.off();'" test->name, test->file);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'pdf(file=\"%s-timings-med.pdf\");z=read.csv(\"%s\");"
		 "plot(z$Delta,z$TimeMedian,xlab=\"Delta\",ylab=\"Median timings (ns)\");"
		 "dev.off();'"; test->name, test->file);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'pdf(file=\"%s-server-timings-avg.pdf\");z=read.csv(\"%s\");"
		 "plot(z$Delta,z$ServerAvg,xlab=\"Delta\",ylab=\"Average timings (ns)\");"
		 "dev.off();'" test->name, test->file);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'pdf(file=\"%s-server-timings-med.pdf\");z=read.csv(\"%s\");"
		 "plot(z$Delta,z$ServerMedian,xlab=\"Delta\",ylab=\"Median timings (ns)\");"
		 "dev.off();'"; test->name, test->file);
	system(prio);
#else
	snprintf(prio, sizeof(prio),
		 "R -e 'z=read.csv(\"%s\");png(filename = \"%s-timings-avg.png\",width=1024,height=1024,units=\"px\","
		 "bg=\"white\");plot(z$Delta,z$TimeAvg,xlab=\"Delta\",ylab=\"Average timings (ns)\");dev.off();'",
		 test->file, test->name);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'z=read.csv(\"%s\");"
		 "png(filename = \"%s-timings-med.png\",width=1024,height=1024,units=\"px\","
		 "bg=\"white\");plot(z$Delta,z$TimeMedian,xlab=\"Delta\",ylab=\"Median timings (ns)\");dev.off();'",
		 test->file, test->name);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'z=read.csv(\"%s\");png(filename = \"%s-server-timings-avg.png\",width=1024,height=1024,units=\"px\","
		 "bg=\"white\");plot(z$Delta,z$ServerAvg,xlab=\"Delta\",ylab=\"Average timings (ns)\");dev.off();'",
		 test->file, test->name);
	system(prio);

	snprintf(prio, sizeof(prio),
		 "R -e 'z=read.csv(\"%s\");"
		 "png(filename = \"%s-server-timings-med.png\",width=1024,height=1024,units=\"px\","
		 "bg=\"white\");plot(z$Delta,z$ServerMedian,xlab=\"Delta\",ylab=\"Median timings (ns)\");dev.off();'",
		 test->file, test->name);
	system(prio);
#endif
	return 0;
}

#endif				/* _WIN32 */
