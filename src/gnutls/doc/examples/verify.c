#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "examples.h"

/* The example below demonstrates the usage of the more powerful
 * gnutls_certificate_verify_peers() which can be used to check
 * the hostname, as well as the key purpose OID of the peer's
 * certificate. */
int verify_certificate_callback(gnutls_session_t session)
{
        unsigned int status;
        int ret, type;
        const char *hostname;
        gnutls_datum_t out;
        gnutls_typed_vdata_st data[2];

        /* read hostname */
        hostname = gnutls_session_get_ptr(session);

        /* This verification function uses the trusted CAs in the credentials
         * structure. So you must have installed one or more CA certificates.
         */
        data[0].type = GNUTLS_DT_DNS_HOSTNAME;
        data[0].data = (void*)hostname;
        data[0].size = 0;

        data[1].type = GNUTLS_DT_KEY_PURPOSE_OID;
        data[1].data = (void*)GNUTLS_KP_TLS_WWW_SERVER;
        data[1].size = 0;
        ret = gnutls_certificate_verify_peers(session, data, 2,
                                              &status);
        if (ret < 0) {
                printf("Error\n");
                return GNUTLS_E_CERTIFICATE_ERROR;
        }

        type = gnutls_certificate_type_get(session);

        ret =
            gnutls_certificate_verification_status_print(status, type,
                                                         &out, 0);
        if (ret < 0) {
                printf("Error\n");
                return GNUTLS_E_CERTIFICATE_ERROR;
        }

        printf("%s", out.data);

        gnutls_free(out.data);

        if (status != 0)        /* Certificate is not trusted */
                return GNUTLS_E_CERTIFICATE_ERROR;

        /* notify gnutls to continue handshake normally */
        return 0;
}
