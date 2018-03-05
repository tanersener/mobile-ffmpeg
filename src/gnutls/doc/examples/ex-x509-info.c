/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include "examples.h"

static const char *bin2hex(const void *bin, size_t bin_size)
{
        static char printable[110];
        const unsigned char *_bin = bin;
        char *print;
        size_t i;

        if (bin_size > 50)
                bin_size = 50;

        print = printable;
        for (i = 0; i < bin_size; i++) {
                sprintf(print, "%.2x ", _bin[i]);
                print += 2;
        }

        return printable;
}

/* This function will print information about this session's peer
 * certificate.
 */
void print_x509_certificate_info(gnutls_session_t session)
{
        char serial[40];
        char dn[256];
        size_t size;
        unsigned int algo, bits;
        time_t expiration_time, activation_time;
        const gnutls_datum_t *cert_list;
        unsigned int cert_list_size = 0;
        gnutls_x509_crt_t cert;
        gnutls_datum_t cinfo;

        /* This function only works for X.509 certificates.
         */
        if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
                return;

        cert_list = gnutls_certificate_get_peers(session, &cert_list_size);

        printf("Peer provided %d certificates.\n", cert_list_size);

        if (cert_list_size > 0) {
                int ret;

                /* we only print information about the first certificate.
                 */
                gnutls_x509_crt_init(&cert);

                gnutls_x509_crt_import(cert, &cert_list[0],
                                       GNUTLS_X509_FMT_DER);

                printf("Certificate info:\n");

                /* This is the preferred way of printing short information about
                   a certificate. */

                ret =
                    gnutls_x509_crt_print(cert, GNUTLS_CRT_PRINT_ONELINE,
                                          &cinfo);
                if (ret == 0) {
                        printf("\t%s\n", cinfo.data);
                        gnutls_free(cinfo.data);
                }

                /* If you want to extract fields manually for some other reason,
                   below are popular example calls. */

                expiration_time =
                    gnutls_x509_crt_get_expiration_time(cert);
                activation_time =
                    gnutls_x509_crt_get_activation_time(cert);

                printf("\tCertificate is valid since: %s",
                       ctime(&activation_time));
                printf("\tCertificate expires: %s",
                       ctime(&expiration_time));

                /* Print the serial number of the certificate.
                 */
                size = sizeof(serial);
                gnutls_x509_crt_get_serial(cert, serial, &size);

                printf("\tCertificate serial number: %s\n",
                       bin2hex(serial, size));

                /* Extract some of the public key algorithm's parameters
                 */
                algo = gnutls_x509_crt_get_pk_algorithm(cert, &bits);

                printf("Certificate public key: %s",
                       gnutls_pk_algorithm_get_name(algo));

                /* Print the version of the X.509
                 * certificate.
                 */
                printf("\tCertificate version: #%d\n",
                       gnutls_x509_crt_get_version(cert));

                size = sizeof(dn);
                gnutls_x509_crt_get_dn(cert, dn, &size);
                printf("\tDN: %s\n", dn);

                size = sizeof(dn);
                gnutls_x509_crt_get_issuer_dn(cert, dn, &size);
                printf("\tIssuer's DN: %s\n", dn);

                gnutls_x509_crt_deinit(cert);

        }
}
