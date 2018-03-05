/*
 * Copyright (C) 2003-2016 Free Software Foundation, Inc.
 * Copyright (C) 2016 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "gnutls_int.h"
#include <libtasn1.h>
#include <datum.h>
#include <global.h>
#include "errors.h"
#include <str.h>
#include <x509.h>
#include <num.h>
#include <x509_b64.h>
#include "x509_int.h"
#include "extras/hex.h"
#include <common.h>

/* TIME functions 
 * Convertions between generalized or UTC time to time_t
 *
 */

/* This is an emulation of the struct tm.
 * Since we do not use libc's functions, we don't need to
 * depend on the libc structure.
 */
typedef struct fake_tm {
	int tm_mon;
	int tm_year;		/* FULL year - ie 1971 */
	int tm_mday;
	int tm_hour;
	int tm_min;
	int tm_sec;
} fake_tm;

/* The mktime_utc function is due to Russ Allbery (rra@stanford.edu),
 * who placed it under public domain:
 */

/* The number of days in each month. 
 */
static const int MONTHDAYS[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

    /* Whether a given year is a leap year. */
#define ISLEAP(year) \
	(((year) % 4) == 0 && (((year) % 100) != 0 || ((year) % 400) == 0))

/*
 **  Given a struct tm representing a calendar time in UTC, convert it to
 **  seconds since epoch.  Returns (time_t) -1 if the time is not
 **  convertable.  Note that this function does not canonicalize the provided
 **  struct tm, nor does it allow out of range values or years before 1970.
 */
static time_t mktime_utc(const struct fake_tm *tm)
{
	time_t result = 0;
	int i;

/* We do allow some ill-formed dates, but we don't do anything special
 * with them and our callers really shouldn't pass them to us.  Do
 * explicitly disallow the ones that would cause invalid array accesses
 * or other algorithm problems. 
 */
	if (tm->tm_mon < 0 || tm->tm_mon > 11 || tm->tm_year < 1970)
		return (time_t) - 1;

/* Convert to a time_t. 
 */
	for (i = 1970; i < tm->tm_year; i++)
		result += 365 + ISLEAP(i);
	for (i = 0; i < tm->tm_mon; i++)
		result += MONTHDAYS[i];
	if (tm->tm_mon > 1 && ISLEAP(tm->tm_year))
		result++;
	result = 24 * (result + tm->tm_mday - 1) + tm->tm_hour;
	result = 60 * result + tm->tm_min;
	result = 60 * result + tm->tm_sec;
	return result;
}


/* this one will parse dates of the form:
 * month|day|hour|minute|sec* (2 chars each)
 * and year is given. Returns a time_t date.
 */
static time_t time2gtime(const char *ttime, int year)
{
	char xx[4];
	struct fake_tm etime;

	if (strlen(ttime) < 8) {
		gnutls_assert();
		return (time_t) - 1;
	}

	etime.tm_year = year;

	/* In order to work with 32 bit
	 * time_t.
	 */
	if (sizeof(time_t) <= 4 && etime.tm_year >= 2038)
		return (time_t) 2145914603;	/* 2037-12-31 23:23:23 */

	if (etime.tm_year < 1970)
		return (time_t) 0;

	xx[2] = 0;

/* get the month
 */
	memcpy(xx, ttime, 2);	/* month */
	etime.tm_mon = atoi(xx) - 1;
	ttime += 2;

/* get the day
 */
	memcpy(xx, ttime, 2);	/* day */
	etime.tm_mday = atoi(xx);
	ttime += 2;

/* get the hour
 */
	memcpy(xx, ttime, 2);	/* hour */
	etime.tm_hour = atoi(xx);
	ttime += 2;

/* get the minutes
 */
	memcpy(xx, ttime, 2);	/* minutes */
	etime.tm_min = atoi(xx);
	ttime += 2;

	if (strlen(ttime) >= 2) {
		memcpy(xx, ttime, 2);
		etime.tm_sec = atoi(xx);
	} else
		etime.tm_sec = 0;

	return mktime_utc(&etime);
}


/* returns a time_t value that contains the given time.
 * The given time is expressed as:
 * YEAR(2)|MONTH(2)|DAY(2)|HOUR(2)|MIN(2)|SEC(2)*
 *
 * (seconds are optional)
 */
static time_t utcTime2gtime(const char *ttime)
{
	char xx[3];
	int year;

	if (strlen(ttime) < 10) {
		gnutls_assert();
		return (time_t) - 1;
	}
	xx[2] = 0;
/* get the year
 */
	memcpy(xx, ttime, 2);	/* year */
	year = atoi(xx);
	ttime += 2;

	if (year > 49)
		year += 1900;
	else
		year += 2000;

	return time2gtime(ttime, year);
}

/* returns a time_t value that contains the given time.
 * The given time is expressed as:
 * YEAR(4)|MONTH(2)|DAY(2)|HOUR(2)|MIN(2)|SEC(2)*
 */
time_t _gnutls_x509_generalTime2gtime(const char *ttime)
{
	char xx[5];
	int year;

	if (strlen(ttime) < 12) {
		gnutls_assert();
		return (time_t) - 1;
	}

	if (strchr(ttime, 'Z') == 0) {
		gnutls_assert();
		/* sorry we don't support it yet
		 */
		return (time_t) - 1;
	}
	xx[4] = 0;

/* get the year
 */
	memcpy(xx, ttime, 4);	/* year */
	year = atoi(xx);
	ttime += 4;

	return time2gtime(ttime, year);
}

/* tag will contain ASN1_TAG_UTCTime or ASN1_TAG_GENERALIZEDTime */
static int
gtime_to_suitable_time(time_t gtime, char *str_time, size_t str_time_size, unsigned *tag)
{
	size_t ret;
	struct tm _tm;

	if (gtime == (time_t)-1
#if SIZEOF_LONG == 8
		|| gtime >= 253402210800
#endif
	 ) {
		if (tag)
			*tag = ASN1_TAG_GENERALIZEDTime;
		snprintf(str_time, str_time_size, "99991231235959Z");
		return 0;
	}

	if (!gmtime_r(&gtime, &_tm)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	if (_tm.tm_year >= 150) {
		if (tag)
			*tag = ASN1_TAG_GENERALIZEDTime;
		ret = strftime(str_time, str_time_size, "%Y%m%d%H%M%SZ", &_tm);
	} else {
		if (tag)
			*tag = ASN1_TAG_UTCTime;
		ret = strftime(str_time, str_time_size, "%y%m%d%H%M%SZ", &_tm);
	}
	if (!ret) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	return 0;
}

static int
gtime_to_generalTime(time_t gtime, char *str_time, size_t str_time_size)
{
	size_t ret;
	struct tm _tm;
	
	if (gtime == (time_t)-1
#if SIZEOF_LONG == 8
		|| gtime >= 253402210800
#endif
	 ) {
		snprintf(str_time, str_time_size, "99991231235959Z");
		return 0;
	}

	if (!gmtime_r(&gtime, &_tm)) {
		gnutls_assert();
		return GNUTLS_E_INTERNAL_ERROR;
	}

	ret = strftime(str_time, str_time_size, "%Y%m%d%H%M%SZ", &_tm);
	if (!ret) {
		gnutls_assert();
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	return 0;
}


/* Extracts the time in time_t from the ASN1_TYPE given. When should
 * be something like "tbsCertList.thisUpdate".
 */
#define MAX_TIME 64
time_t _gnutls_x509_get_time(ASN1_TYPE c2, const char *where, int force_general)
{
	char ttime[MAX_TIME];
	char name[128];
	time_t c_time = (time_t) - 1;
	int len, result;

	len = sizeof(ttime) - 1;
	result = asn1_read_value(c2, where, ttime, &len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return (time_t) (-1);
	}

	if (force_general != 0) {
		c_time = _gnutls_x509_generalTime2gtime(ttime);
	} else {
		_gnutls_str_cpy(name, sizeof(name), where);

		/* choice */
		if (strcmp(ttime, "generalTime") == 0) {
			if (name[0] == 0)
				_gnutls_str_cpy(name, sizeof(name),
						"generalTime");
			else
				_gnutls_str_cat(name, sizeof(name),
						".generalTime");
			len = sizeof(ttime) - 1;
			result = asn1_read_value(c2, name, ttime, &len);
			if (result == ASN1_SUCCESS)
				c_time =
				    _gnutls_x509_generalTime2gtime(ttime);
		} else {	/* UTCTIME */
			if (name[0] == 0)
				_gnutls_str_cpy(name, sizeof(name), "utcTime");
			else
				_gnutls_str_cat(name, sizeof(name), ".utcTime");
			len = sizeof(ttime) - 1;
			result = asn1_read_value(c2, name, ttime, &len);
			if (result == ASN1_SUCCESS)
				c_time = utcTime2gtime(ttime);
		}

		/* We cannot handle dates after 2031 in 32 bit machines.
		 * a time_t of 64bits has to be used.
		 */
		if (result != ASN1_SUCCESS) {
			gnutls_assert();
			return (time_t) (-1);
		}
	}

	return c_time;
}

/* Sets the time in time_t in the ASN1_TYPE given. Where should
 * be something like "tbsCertList.thisUpdate".
 */
int
_gnutls_x509_set_time(ASN1_TYPE c2, const char *where, time_t tim,
		      int force_general)
{
	char str_time[MAX_TIME];
	char name[128];
	int result, len;
	unsigned tag;

	if (force_general != 0) {
		result =
		    gtime_to_generalTime(tim, str_time, sizeof(str_time));
		if (result < 0)
			return gnutls_assert_val(result);
		len = strlen(str_time);
		result = asn1_write_value(c2, where, str_time, len);
		if (result != ASN1_SUCCESS)
			return gnutls_assert_val(_gnutls_asn2err(result));

		return 0;
	}

	result = gtime_to_suitable_time(tim, str_time, sizeof(str_time), &tag);
	if (result < 0) {
		gnutls_assert();
		return result;
	}

	_gnutls_str_cpy(name, sizeof(name), where);
	if (tag == ASN1_TAG_UTCTime) {
		if ((result = asn1_write_value(c2, where, "utcTime", 1)) < 0) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}
		_gnutls_str_cat(name, sizeof(name), ".utcTime");
	} else {
		if ((result = asn1_write_value(c2, where, "generalTime", 1)) < 0) {
			gnutls_assert();
			return _gnutls_asn2err(result);
		}
		_gnutls_str_cat(name, sizeof(name), ".generalTime");
	}

	len = strlen(str_time);
	result = asn1_write_value(c2, name, str_time, len);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	return 0;
}

/* This will set a DER encoded Time element. To be used in fields
 * which are of the ANY.
 */
int
_gnutls_x509_set_raw_time(ASN1_TYPE c2, const char *where, time_t tim)
{
	char str_time[MAX_TIME];
	uint8_t buf[128];
	int result, len, der_len;
	unsigned tag;

	result =
	    gtime_to_suitable_time(tim, str_time, sizeof(str_time), &tag);
	if (result < 0)
		return gnutls_assert_val(result);
	len = strlen(str_time);

	buf[0] = tag;
	asn1_length_der(len, buf+1, &der_len);

	if ((unsigned)len > sizeof(buf)-der_len-1) {
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);
	}

	memcpy(buf+1+der_len, str_time, len);

	result = asn1_write_value(c2, where, buf, len+1+der_len);
	if (result != ASN1_SUCCESS)
		return gnutls_assert_val(_gnutls_asn2err(result));
	return 0;
}

