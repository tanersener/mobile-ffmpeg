/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "examples.h"

/* This function will print some details of the
 * given session.
 */
int print_info(gnutls_session_t session)
{
        const char *tmp;
        gnutls_credentials_type_t cred;
        gnutls_kx_algorithm_t kx;
        int dhe, ecdh;

        dhe = ecdh = 0;

        /* print the key exchange's algorithm name
         */
        kx = gnutls_kx_get(session);
        tmp = gnutls_kx_get_name(kx);
        printf("- Key Exchange: %s\n", tmp);

        /* Check the authentication type used and switch
         * to the appropriate.
         */
        cred = gnutls_auth_get_type(session);
        switch (cred) {
        case GNUTLS_CRD_IA:
                printf("- TLS/IA session\n");
                break;


#ifdef ENABLE_SRP
        case GNUTLS_CRD_SRP:
                printf("- SRP session with username %s\n",
                       gnutls_srp_server_get_username(session));
                break;
#endif

        case GNUTLS_CRD_PSK:
                /* This returns NULL in server side.
                 */
                if (gnutls_psk_client_get_hint(session) != NULL)
                        printf("- PSK authentication. PSK hint '%s'\n",
                               gnutls_psk_client_get_hint(session));
                /* This returns NULL in client side.
                 */
                if (gnutls_psk_server_get_username(session) != NULL)
                        printf("- PSK authentication. Connected as '%s'\n",
                               gnutls_psk_server_get_username(session));

                if (kx == GNUTLS_KX_ECDHE_PSK)
                        ecdh = 1;
                else if (kx == GNUTLS_KX_DHE_PSK)
                        dhe = 1;
                break;

        case GNUTLS_CRD_ANON:  /* anonymous authentication */

                printf("- Anonymous authentication.\n");
                if (kx == GNUTLS_KX_ANON_ECDH)
                        ecdh = 1;
                else if (kx == GNUTLS_KX_ANON_DH)
                        dhe = 1;
                break;

        case GNUTLS_CRD_CERTIFICATE:   /* certificate authentication */

                /* Check if we have been using ephemeral Diffie-Hellman.
                 */
                if (kx == GNUTLS_KX_DHE_RSA || kx == GNUTLS_KX_DHE_DSS)
                        dhe = 1;
                else if (kx == GNUTLS_KX_ECDHE_RSA
                         || kx == GNUTLS_KX_ECDHE_ECDSA)
                        ecdh = 1;

                /* if the certificate list is available, then
                 * print some information about it.
                 */
                print_x509_certificate_info(session);

        }                       /* switch */

        if (ecdh != 0)
                printf("- Ephemeral ECDH using curve %s\n",
                       gnutls_ecc_curve_get_name(gnutls_ecc_curve_get
                                                 (session)));
        else if (dhe != 0)
                printf("- Ephemeral DH using prime of %d bits\n",
                       gnutls_dh_get_prime_bits(session));

        /* print the protocol's name (ie TLS 1.0) 
         */
        tmp =
            gnutls_protocol_get_name(gnutls_protocol_get_version(session));
        printf("- Protocol: %s\n", tmp);

        /* print the certificate type of the peer.
         * ie X.509
         */
        tmp =
            gnutls_certificate_type_get_name(gnutls_certificate_type_get
                                             (session));

        printf("- Certificate Type: %s\n", tmp);

        /* print the compression algorithm (if any)
         */
        tmp = gnutls_compression_get_name(gnutls_compression_get(session));
        printf("- Compression: %s\n", tmp);

        /* print the name of the cipher used.
         * ie 3DES.
         */
        tmp = gnutls_cipher_get_name(gnutls_cipher_get(session));
        printf("- Cipher: %s\n", tmp);

        /* Print the MAC algorithms name.
         * ie SHA1
         */
        tmp = gnutls_mac_get_name(gnutls_mac_get(session));
        printf("- MAC: %s\n", tmp);

        return 0;
}
