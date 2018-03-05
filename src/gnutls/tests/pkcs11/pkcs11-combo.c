/*
 * Copyright (C) 2014 Red Hat
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
#include <string.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>

/* Tests whether the combination of a trust module + additional CAs
 * in a trust list would work.
 */

#include "../utils.h"
#include "../test-chains.h"
#include "softhsm.h"

#define NAME "softhsm-combo"
#define CONFIG NAME".config"

/* These CAs have the same DN */
static const char *ca_list[MAX_CHAIN] = {
"-----BEGIN CERTIFICATE-----\n"
"MIIHSjCCBjKgAwIBAgIKYRHt9wABAAAAFTANBgkqhkiG9w0BAQUFADBSMQswCQYD\n"
"VQQGEwJVUzEaMBgGA1UEChMRSW50ZWwgQ29ycG9yYXRpb24xJzAlBgNVBAMTHklu\n"
"dGVsIEludHJhbmV0IEJhc2ljIFBvbGljeSBDQTAeFw0xMzAyMDQyMTUyMThaFw0x\n"
"ODA1MjQxOTU5MzlaMFYxCzAJBgNVBAYTAlVTMRowGAYDVQQKExFJbnRlbCBDb3Jw\n"
"b3JhdGlvbjErMCkGA1UEAxMiSW50ZWwgSW50cmFuZXQgQmFzaWMgSXNzdWluZyBD\n"
"QSAyQjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALn3ogjraWSmK5Wb\n"
"/4e9mENA1F36FBVemaG7L93ZhRRXq4UV0PQM5/4TOe9KAaOlX+a2cuULeeUtN9Rk\n"
"V/nHAVzSWlqc/NTMJfuI/1AD7ICNejQFYLxDMXGjR7eAHtiMz0iTMp9u6YTw4WXh\n"
"WffqTPiqUZ6DEWsMic9dM9yw/JqzycKClLcTD1OCvtw7Fx4tNTu6/ngrYJcTo29e\n"
"BBh/DupgtgnYPYuExEkHmucb4VIDdjfRkPo/BdNqrUSYfYqnUDj5mH+hPzIgppsZ\n"
"Rw0S5PUZGuC1f+Zok+4vZPR+hGG3Pdm2LTUEWSnurlhyfBoM+0yxeHsmL9aHU7zt\n"
"EIzVmKUCAwEAAaOCBBwwggQYMBIGCSsGAQQBgjcVAQQFAgMCAAIwIwYJKwYBBAGC\n"
"NxUCBBYEFMqHyYZOx6LYwRwZ+5vjOyIl9hENMB0GA1UdDgQWBBQ4Y3b6tgU6qVlP\n"
"SoeNoIO3fpE6CzAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMC\n"
"AYYwEgYDVR0TAQH/BAgwBgEB/wIBADAfBgNVHSMEGDAWgBRp6zCRHAOAgE4RFYhG\n"
"pOJBmtNpHzCCAaIGA1UdHwSCAZkwggGVMIIBkaCCAY2gggGJhlFodHRwOi8vd3d3\n"
"LmludGVsLmNvbS9yZXBvc2l0b3J5L0NSTC9JbnRlbCUyMEludHJhbmV0JTIwQmFz\n"
"aWMlMjBQb2xpY3klMjBDQSgxKS5jcmyGWmh0dHA6Ly9jZXJ0aWZpY2F0ZXMuaW50\n"
"ZWwuY29tL3JlcG9zaXRvcnkvQ1JML0ludGVsJTIwSW50cmFuZXQlMjBCYXNpYyUy\n"
"MFBvbGljeSUyMENBKDEpLmNybIaB12xkYXA6Ly8vQ049SW50ZWwlMjBJbnRyYW5l\n"
"dCUyMEJhc2ljJTIwUG9saWN5JTIwQ0EoMSksQ049bWNzaWJwY2EsQ049Q0RQLENO\n"
"PVB1YmxpYyUyMEtleSUyMFNlcnZpY2VzLENOPVNlcnZpY2VzLENOPUNvbmZpZ3Vy\n"
"YXRpb24sREM9Y29ycCxEQz1pbnRlbCxEQz1jb20/Y2VydGlmaWNhdGVSZXZvY2F0\n"
"aW9uTGlzdD9iYXNlP29iamVjdENsYXNzPWNSTERpc3RyaWJ1dGlvblBvaW50MIIB\n"
"uQYIKwYBBQUHAQEEggGrMIIBpzBmBggrBgEFBQcwAoZaaHR0cDovL3d3dy5pbnRl\n"
"bC5jb20vcmVwb3NpdG9yeS9jZXJ0aWZpY2F0ZXMvSW50ZWwlMjBJbnRyYW5ldCUy\n"
"MEJhc2ljJTIwUG9saWN5JTIwQ0EoMSkuY3J0MG8GCCsGAQUFBzAChmNodHRwOi8v\n"
"Y2VydGlmaWNhdGVzLmludGVsLmNvbS9yZXBvc2l0b3J5L2NlcnRpZmljYXRlcy9J\n"
"bnRlbCUyMEludHJhbmV0JTIwQmFzaWMlMjBQb2xpY3klMjBDQSgxKS5jcnQwgcsG\n"
"CCsGAQUFBzAChoG+bGRhcDovLy9DTj1JbnRlbCUyMEludHJhbmV0JTIwQmFzaWMl\n"
"MjBQb2xpY3klMjBDQSxDTj1BSUEsQ049UHVibGljJTIwS2V5JTIwU2VydmljZXMs\n"
"Q049U2VydmljZXMsQ049Q29uZmlndXJhdGlvbixEQz1jb3JwLERDPWludGVsLERD\n"
"PWNvbT9jQUNlcnRpZmljYXRlP2Jhc2U/b2JqZWN0Q2xhc3M9Y2VydGlmaWNhdGlv\n"
"bkF1dGhvcml0eTANBgkqhkiG9w0BAQUFAAOCAQEAsj8cHt2jSAmnIGulE9jXooAc\n"
"qH2xehlI+ko/al+nDnBzbjDYYjVS52XitYg8JGo6j72ijiGlGb/03FcQJRBZmUH6\n"
"znktx2rGTm4IdjL8quhvHthlzXXCozL8GMeeOuZ5rzHlhapKx764a5RuZtyx89uS\n"
"9cECon6oLGesXjFJ8Xrq6ecHZrQwJUpmvZalwvloKACAWqBh8yV12WDnUNZhtp8N\n"
"8rqeJZoy/lXGnTxsSSodO/5Y/CxYJM4W6u4WgvXNJSjO/0qWvb64S+pVLjBzwI+Y\n"
"X6oLqmBovRp1lGPOLjkXZi3EKDR8DmzhtpJq2677RtYowewnFedQ+exH9cXoJw==\n"
"-----END CERTIFICATE-----",
"-----BEGIN CERTIFICATE-----\n"
"MIIHSjCCBjKgAwIBAgIKYRXxrQABAAAAETANBgkqhkiG9w0BAQUFADBSMQswCQYD\n"
"VQQGEwJVUzEaMBgGA1UEChMRSW50ZWwgQ29ycG9yYXRpb24xJzAlBgNVBAMTHklu\n"
"dGVsIEludHJhbmV0IEJhc2ljIFBvbGljeSBDQTAeFw0wOTA1MTUxODQyNDVaFw0x\n"
"NTA1MTUxODUyNDVaMFYxCzAJBgNVBAYTAlVTMRowGAYDVQQKExFJbnRlbCBDb3Jw\n"
"b3JhdGlvbjErMCkGA1UEAxMiSW50ZWwgSW50cmFuZXQgQmFzaWMgSXNzdWluZyBD\n"
"QSAyQjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKbJOXtXYgfyoch6\n"
"ip5SSjijOXvpIjBxbTl5EGH/VYHmpM2O6SRlKh/uy77QS9m84sRWCJLr8cWwX9oH\n"
"qSmIylgcWvDpVNHx4v506DTTrbK0sbYRQYXRajOzJKeTt7NLeLrngyl45FrI9VAT\n"
"3yqp/2BCG1dUwcBha3dB2UbTkFOMt9o/gqoL6KvgswYMs/oGc/OIjeozdYuhnBT2\n"
"YlT9Ge5pfhOJWXh4DJbxnTmWwRUKq0MXFn0S00KQ/BZOTkc/5DibUmbmMrYi8ra4\n"
"Z2bpnoTq0WNA99O2Lk8IgmkqPdi6HwZwKCE/x01qwP8zo76rvN8sbW9pj2WzS1WF\n"
"tSDPeZECAwEAAaOCBBwwggQYMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYE\n"
"FPwbdyds7Cm03lobLKmI6q59npi+MAsGA1UdDwQEAwIBhjASBgkrBgEEAYI3FQEE\n"
"BQIDAQABMCMGCSsGAQQBgjcVAgQWBBRT1n27C6cZL4QFHaUX2nFSCPxhtTAZBgkr\n"
"BgEEAYI3FAIEDB4KAFMAdQBiAEMAQTAfBgNVHSMEGDAWgBRp6zCRHAOAgE4RFYhG\n"
"pOJBmtNpHzCCAaIGA1UdHwSCAZkwggGVMIIBkaCCAY2gggGJhlFodHRwOi8vd3d3\n"
"LmludGVsLmNvbS9yZXBvc2l0b3J5L0NSTC9JbnRlbCUyMEludHJhbmV0JTIwQmFz\n"
"aWMlMjBQb2xpY3klMjBDQSgxKS5jcmyGWmh0dHA6Ly9jZXJ0aWZpY2F0ZXMuaW50\n"
"ZWwuY29tL3JlcG9zaXRvcnkvQ1JML0ludGVsJTIwSW50cmFuZXQlMjBCYXNpYyUy\n"
"MFBvbGljeSUyMENBKDEpLmNybIaB12xkYXA6Ly8vQ049SW50ZWwlMjBJbnRyYW5l\n"
"dCUyMEJhc2ljJTIwUG9saWN5JTIwQ0EoMSksQ049bWNzaWJwY2EsQ049Q0RQLENO\n"
"PVB1YmxpYyUyMEtleSUyMFNlcnZpY2VzLENOPVNlcnZpY2VzLENOPUNvbmZpZ3Vy\n"
"YXRpb24sREM9Y29ycCxEQz1pbnRlbCxEQz1jb20/Y2VydGlmaWNhdGVSZXZvY2F0\n"
"aW9uTGlzdD9iYXNlP29iamVjdENsYXNzPWNSTERpc3RyaWJ1dGlvblBvaW50MIIB\n"
"uQYIKwYBBQUHAQEEggGrMIIBpzBmBggrBgEFBQcwAoZaaHR0cDovL3d3dy5pbnRl\n"
"bC5jb20vcmVwb3NpdG9yeS9jZXJ0aWZpY2F0ZXMvSW50ZWwlMjBJbnRyYW5ldCUy\n"
"MEJhc2ljJTIwUG9saWN5JTIwQ0EoMSkuY3J0MG8GCCsGAQUFBzAChmNodHRwOi8v\n"
"Y2VydGlmaWNhdGVzLmludGVsLmNvbS9yZXBvc2l0b3J5L2NlcnRpZmljYXRlcy9J\n"
"bnRlbCUyMEludHJhbmV0JTIwQmFzaWMlMjBQb2xpY3klMjBDQSgxKS5jcnQwgcsG\n"
"CCsGAQUFBzAChoG+bGRhcDovLy9DTj1JbnRlbCUyMEludHJhbmV0JTIwQmFzaWMl\n"
"MjBQb2xpY3klMjBDQSxDTj1BSUEsQ049UHVibGljJTIwS2V5JTIwU2VydmljZXMs\n"
"Q049U2VydmljZXMsQ049Q29uZmlndXJhdGlvbixEQz1jb3JwLERDPWludGVsLERD\n"
"PWNvbT9jQUNlcnRpZmljYXRlP2Jhc2U/b2JqZWN0Q2xhc3M9Y2VydGlmaWNhdGlv\n"
"bkF1dGhvcml0eTANBgkqhkiG9w0BAQUFAAOCAQEArlAkrJXyMCssqAJT3PqnY7wt\n"
"sirq1fTMrVrHdmkpBKDXBQnDcTW1zfZtOPV/QDm3UsFwDBbGq+j/7U9qZ1zYHkv+\n"
"wrBpeFM6dlca/sgegGGAhYnQQwmlSzNXCKHMBltMjT61X8rVjyt1XJnucgat9rnT\n"
"2j8pztqoViVnORsGfT6DDB/bz/6bFKw4FMp1wDaJI7dKh5NUggvH36owTWI7JUvq\n"
"yJ8OI2qmjXrlqGexfwvltIkEk8xzuMIHWQoR8sERL2qf3nb2VYq1s1LbH5uCkZ0l\n"
"w/xgwFbbwjaGJ3TFOmkVKYU77nXSkfK9EXae0UZRU0WmX4t5NNt8jiL56TPpsw==\n"
"-----END CERTIFICATE-----\n",
"-----BEGIN CERTIFICATE-----\n"
"MIIHIzCCBgugAwIBAgIKYRok3wABAAAADDANBgkqhkiG9w0BAQUFADBSMQswCQYD\n"
"VQQGEwJVUzEaMBgGA1UEChMRSW50ZWwgQ29ycG9yYXRpb24xJzAlBgNVBAMTHklu\n"
"dGVsIEludHJhbmV0IEJhc2ljIFBvbGljeSBDQTAeFw0wNjA1MjQxOTU2MDFaFw0x\n"
"MjA1MjQyMDA2MDFaMFYxCzAJBgNVBAYTAlVTMRowGAYDVQQKExFJbnRlbCBDb3Jw\n"
"b3JhdGlvbjErMCkGA1UEAxMiSW50ZWwgSW50cmFuZXQgQmFzaWMgSXNzdWluZyBD\n"
"QSAyQjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANE2pFSB0XqXoRWF\n"
"N7bzDesBAcTGEqcr6GVA+sMcJ5Vt17S8vGesmO2RgP6I49Q58nIhUnT054arUlOx\n"
"NKYbAEiVyGOK5zV2mZS4oW2UazfcpsV1uuO3j02UbzX+qcxQdNqoAHxwoB4nRJuU\n"
"Ijio45jWAssDbD8IKHZpmqRI5wUzbibkWnTZEc0YFO6iF40sNtqVr+uInP07PkQn\n"
"1Ttkyw6isa5Dhcyq6lTVOjnlj29bFYbZxN1uuDnTpUMVeov8oQv5wLyLrDVd1sMg\n"
"Njr2oofepZ8KjF3DKCkfsUekCHA9Pr2K/4hStd/nSwvIdNjCjfznqYadkB6wQ99a\n"
"hTX4uJkCAwEAAaOCA/UwggPxMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYE\n"
"FJunwCR+/af8p76CGTyhUZc3l/4DMAsGA1UdDwQEAwIBhjAQBgkrBgEEAYI3FQEE\n"
"AwIBADAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTAfBgNVHSMEGDAWgBRp6zCR\n"
"HAOAgE4RFYhGpOJBmtNpHzCCAaIGA1UdHwSCAZkwggGVMIIBkaCCAY2gggGJhlFo\n"
"dHRwOi8vd3d3LmludGVsLmNvbS9yZXBvc2l0b3J5L0NSTC9JbnRlbCUyMEludHJh\n"
"bmV0JTIwQmFzaWMlMjBQb2xpY3klMjBDQSgxKS5jcmyGWmh0dHA6Ly9jZXJ0aWZp\n"
"Y2F0ZXMuaW50ZWwuY29tL3JlcG9zaXRvcnkvQ1JML0ludGVsJTIwSW50cmFuZXQl\n"
"MjBCYXNpYyUyMFBvbGljeSUyMENBKDEpLmNybIaB12xkYXA6Ly8vQ049SW50ZWwl\n"
"MjBJbnRyYW5ldCUyMEJhc2ljJTIwUG9saWN5JTIwQ0EoMSksQ049bWNzaWJwY2Es\n"
"Q049Q0RQLENOPVB1YmxpYyUyMEtleSUyMFNlcnZpY2VzLENOPVNlcnZpY2VzLENO\n"
"PUNvbmZpZ3VyYXRpb24sREM9Y29ycCxEQz1pbnRlbCxEQz1jb20/Y2VydGlmaWNh\n"
"dGVSZXZvY2F0aW9uTGlzdD9iYXNlP29iamVjdENsYXNzPWNSTERpc3RyaWJ1dGlv\n"
"blBvaW50MIIBuQYIKwYBBQUHAQEEggGrMIIBpzBmBggrBgEFBQcwAoZaaHR0cDov\n"
"L3d3dy5pbnRlbC5jb20vcmVwb3NpdG9yeS9jZXJ0aWZpY2F0ZXMvSW50ZWwlMjBJ\n"
"bnRyYW5ldCUyMEJhc2ljJTIwUG9saWN5JTIwQ0EoMSkuY3J0MG8GCCsGAQUFBzAC\n"
"hmNodHRwOi8vY2VydGlmaWNhdGVzLmludGVsLmNvbS9yZXBvc2l0b3J5L2NlcnRp\n"
"ZmljYXRlcy9JbnRlbCUyMEludHJhbmV0JTIwQmFzaWMlMjBQb2xpY3klMjBDQSgx\n"
"KS5jcnQwgcsGCCsGAQUFBzAChoG+bGRhcDovLy9DTj1JbnRlbCUyMEludHJhbmV0\n"
"JTIwQmFzaWMlMjBQb2xpY3klMjBDQSxDTj1BSUEsQ049UHVibGljJTIwS2V5JTIw\n"
"U2VydmljZXMsQ049U2VydmljZXMsQ049Q29uZmlndXJhdGlvbixEQz1jb3JwLERD\n"
"PWludGVsLERDPWNvbT9jQUNlcnRpZmljYXRlP2Jhc2U/b2JqZWN0Q2xhc3M9Y2Vy\n"
"dGlmaWNhdGlvbkF1dGhvcml0eTANBgkqhkiG9w0BAQUFAAOCAQEAe3SmN0lsGF0h\n"
"zq+NANnUD4YJS31UqreVm4kJv07+9CTBtlB0AVqJ2RcjRosdQmrbhx7R0WwcXSdR\n"
"QnRGhaoDVRNehKiz3Grp6ehJr9LInhCp6WtOeKRlOSb2xgRDJCtzCi07TuAb9h2I\n"
"urpmndeA4NEbPYL1GYEBpKYawUcFCq5yTv0YgZXy53DdBDv9ygRWYGEk7/gPgvCu\n"
"2O1GNs9n25goy+3/aMkHnUyl3MOtiooXJR7eKOEgTPHNe42LQ9KuUz5SoZQN8vSL\n"
"r49IRDC4dgMkGvsC5h0+ftixQ66ni6QJe6SNcpSZrpW5vBE9J+vtDI0gTyq2SYPo\n"
"0fiS3V8p4g==\n"
"-----END CERTIFICATE-----\n",
NULL};

/* GnuTLS internally calls time() to find out the current time when
   verifying certificates.  To avoid a time bomb, we hard code the
   current time.  This should work fine on systems where the library
   call to time is resolved at run-time.  */
static time_t mytime(time_t * t)
{
	time_t then = 1256803113;

	if (t)
		*t = then;

	return then;
}

#define PIN "1234"

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "|<%d>| %s", level, str);
}

static
int pin_func(void* userdata, int attempt, const char* url, const char *label,
		unsigned flags, char *pin, size_t pin_max)
{
	if (attempt == 0) {
		strcpy(pin, PIN);
		return 0;
	}
	return -1;
}

void doit(void)
{
	char buf[128];
	int exit_val = 0;
	int ret;
	unsigned j;
	const char *lib, *bin;
	gnutls_x509_crt_t issuer = NULL;
	gnutls_x509_trust_list_t tl;
	gnutls_x509_crt_t certs[MAX_CHAIN];
	gnutls_x509_crt_t end, ca;
	unsigned verify_status = 0;
	gnutls_datum_t tmp;

	/* The overloading of time() seems to work in linux (ELF?)
	 * systems only. Disable it on windows.
	 */
#ifdef _WIN32
	exit(77);
#endif
	bin = softhsm_bin();

	lib = softhsm_lib();

	ret = global_init();
	if (ret != 0) {
		fail("%d: %s\n", ret, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_pkcs11_set_pin_function(pin_func, NULL);
	gnutls_global_set_time_function(mytime);
	gnutls_global_set_log_function(tls_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	set_softhsm_conf(CONFIG);
	snprintf(buf, sizeof(buf), "%s --init-token --slot 0 --label test --so-pin "PIN" --pin "PIN, bin);
	system(buf);

	ret = gnutls_pkcs11_add_provider(lib, "trusted");
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	for (j = 0; ca_list[j]; j++) {
		if (debug > 2)
			printf("\tAdding certificate %d...",
			       (int) j);

		ret = gnutls_x509_crt_init(&certs[j]);
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_init[%d,%d]: %s\n",
				(int) 3, (int) j,
				gnutls_strerror(ret));
			exit(1);
		}

		tmp.data = (unsigned char *) ca_list[j];
		tmp.size = strlen(ca_list[j]);

		ret =
		    gnutls_x509_crt_import(certs[j], &tmp,
					   GNUTLS_X509_FMT_PEM);
		if (debug > 2)
			printf("done\n");
		if (ret < 0) {
			fprintf(stderr,
				"gnutls_x509_crt_import[%d]: %s\n",
				(int) j,
				gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_crt_print(certs[j],
				      GNUTLS_CRT_PRINT_ONELINE,
				      &tmp);
		if (debug)
			printf("\tCertificate %d: %.*s\n", (int) j,
			       tmp.size, tmp.data);
		gnutls_free(tmp.data);
	}

	if (debug > 2)
		printf("\tAdding end certificate...");

	ret = gnutls_x509_crt_init(&end);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	tmp.data = (unsigned char *) v1_root_check[0];
	tmp.size = strlen(v1_root_check[0]);

	ret =
	    gnutls_x509_crt_import(end, &tmp, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_print(end, GNUTLS_CRT_PRINT_ONELINE, &tmp);
	if (debug)
		printf("\tEnd Certificate: %.*s\n", tmp.size,
		       tmp.data);
	gnutls_free(tmp.data);

	ret = gnutls_x509_crt_init(&ca);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_init: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	tmp.data = (unsigned char *) v1_root_check[1];
	tmp.size = strlen(v1_root_check[1]);

	ret =
	    gnutls_x509_crt_import(ca, &tmp, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_import: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_print(end, GNUTLS_CRT_PRINT_ONELINE, &tmp);
	if (debug)
		printf("\tCA Certificate: %.*s\n", tmp.size,
		       tmp.data);
	gnutls_free(tmp.data);

	if (debug > 2)
		printf("done\n");


	if (debug)
		printf("\tChecking presence and verification...");

	/* initialize softhsm token */
	ret = gnutls_pkcs11_token_init(SOFTHSM_URL, PIN, "test");
	if (ret < 0) {
		fail("gnutls_pkcs11_token_init\n");
		exit(1);
	}

	/* write CA certificate to softhsm */
	for (j = 0; ca_list[j]; j++) {
		char name[64];
		snprintf(name, sizeof(name), "test-ca%d", j);
		ret = gnutls_pkcs11_copy_x509_crt(SOFTHSM_URL, certs[j], name, GNUTLS_PKCS11_OBJ_FLAG_MARK_TRUSTED|GNUTLS_PKCS11_OBJ_FLAG_LOGIN_SO);
		if (ret < 0) {
			fail("gnutls_pkcs11_copy_x509_crt: %s\n", gnutls_strerror(ret));
			exit(1);
		}
	}

	gnutls_x509_trust_list_init(&tl, 0);

	ret = gnutls_x509_trust_list_add_trust_file(tl, SOFTHSM_URL, NULL, 0, 0, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_trust_file\n");
		exit(1);
	}

	ret = gnutls_x509_trust_list_add_cas(tl, &ca, 1, 0);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_add_cas\n");
		exit(1);
	}

	/* extract the issuer of the certificate */
	ret = gnutls_x509_trust_list_get_issuer(tl, end, &issuer, GNUTLS_TL_GET_COPY);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_get_issuer should have succeeded\n");
		exit(1);
	}
	gnutls_x509_crt_deinit(issuer);

	ret = gnutls_pkcs11_crt_is_known(SOFTHSM_URL, ca, GNUTLS_PKCS11_OBJ_FLAG_COMPARE_KEY|GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_TRUSTED);
	if (ret != 0) {
		fail("gnutls_pkcs11_crt_is_known should have failed!\n");
		exit(1);
	}

	ret = gnutls_x509_trust_list_verify_crt2(tl, &end, 1,
						NULL, 0,
						GNUTLS_VERIFY_DISABLE_TIME_CHECKS, &verify_status, NULL);
	if (ret < 0) {
		fail("gnutls_x509_trust_list_verify_crt2 should have succeeded\n");
		exit(1);
	}

	if (verify_status != 0) {
		fail("verification should have succeeded: %.2x\n", verify_status);
		exit(1);
	}

	if (debug)
		printf("\tCleanup...");

	gnutls_x509_trust_list_deinit(tl, 0);
	gnutls_x509_crt_deinit(ca);
	gnutls_x509_crt_deinit(end);
	for (j = 0; ca_list[j]; j++) {
		gnutls_x509_crt_deinit(certs[j]);
	}
	if (debug)
		printf("done\n\n\n");

	gnutls_global_deinit();

	if (debug)
		printf("Exit status...%d\n", exit_val);
	remove(CONFIG);

	exit(exit_val);
}
