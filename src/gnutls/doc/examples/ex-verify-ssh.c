/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>
#include "examples.h"

#define CHECK(x) assert((x)>=0)

/* This function will verify the peer's certificate, check
 * if the hostname matches. In addition it will perform an
 * SSH-style authentication, where ultimately trusted keys
 * are only the keys that have been seen before.
 */
int _ssh_verify_certificate_callback(gnutls_session_t session)
{
        unsigned int status;
        const gnutls_datum_t *cert_list;
        unsigned int cert_list_size;
        int ret, type;
        gnutls_datum_t out;
        const char *hostname;

        /* read hostname */
        hostname = gnutls_session_get_ptr(session);

        /* This verification function uses the trusted CAs in the credentials
         * structure. So you must have installed one or more CA certificates.
         */
        CHECK(gnutls_certificate_verify_peers3(session, hostname, &status));

        type = gnutls_certificate_type_get(session);

        CHECK(gnutls_certificate_verification_status_print(status,
                                                           type, &out, 0));
        printf("%s", out.data);

        gnutls_free(out.data);

        if (status != 0)        /* Certificate is not trusted */
                return GNUTLS_E_CERTIFICATE_ERROR;

        /* Do SSH verification */
        cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
        if (cert_list == NULL) {
                printf("No certificate was found!\n");
                return GNUTLS_E_CERTIFICATE_ERROR;
        }

        /* service may be obtained alternatively using getservbyport() */
        ret = gnutls_verify_stored_pubkey(NULL, NULL, hostname, "https",
                                          type, &cert_list[0], 0);
        if (ret == GNUTLS_E_NO_CERTIFICATE_FOUND) {
                printf("Host %s is not known.", hostname);
                if (status == 0)
                        printf("Its certificate is valid for %s.\n",
                               hostname);

                /* the certificate must be printed and user must be asked on
                 * whether it is trustworthy. --see gnutls_x509_crt_print() */

                /* if not trusted */
                return GNUTLS_E_CERTIFICATE_ERROR;
        } else if (ret == GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
                printf
                    ("Warning: host %s is known but has another key associated.",
                     hostname);
                printf
                    ("It might be that the server has multiple keys, or you are under attack\n");
                if (status == 0)
                        printf("Its certificate is valid for %s.\n",
                               hostname);

                /* the certificate must be printed and user must be asked on
                 * whether it is trustworthy. --see gnutls_x509_crt_print() */

                /* if not trusted */
                return GNUTLS_E_CERTIFICATE_ERROR;
        } else if (ret < 0) {
                printf("gnutls_verify_stored_pubkey: %s\n",
                       gnutls_strerror(ret));
                return ret;
        }

        /* user trusts the key -> store it */
        if (ret != 0) {
                CHECK(gnutls_store_pubkey(NULL, NULL, hostname, "https",
                                          type, &cert_list[0], 0, 0));
        }

        /* notify gnutls to continue handshake normally */
        return 0;
}
