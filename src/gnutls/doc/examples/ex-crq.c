/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <time.h>

/* This example will generate a private key and a certificate
 * request.
 */

int main(void)
{
        gnutls_x509_crq_t crq;
        gnutls_x509_privkey_t key;
        unsigned char buffer[10 * 1024];
        size_t buffer_size = sizeof(buffer);
        unsigned int bits;

        gnutls_global_init();

        /* Initialize an empty certificate request, and
         * an empty private key.
         */
        gnutls_x509_crq_init(&crq);

        gnutls_x509_privkey_init(&key);

        /* Generate an RSA key of moderate security.
         */
        bits =
            gnutls_sec_param_to_pk_bits(GNUTLS_PK_RSA,
                                        GNUTLS_SEC_PARAM_MEDIUM);
        gnutls_x509_privkey_generate(key, GNUTLS_PK_RSA, bits, 0);

        /* Add stuff to the distinguished name
         */
        gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_COUNTRY_NAME,
                                      0, "GR", 2);

        gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_X520_COMMON_NAME,
                                      0, "Nikos", strlen("Nikos"));

        /* Set the request version.
         */
        gnutls_x509_crq_set_version(crq, 1);

        /* Set a challenge password.
         */
        gnutls_x509_crq_set_challenge_password(crq,
                                               "something to remember here");

        /* Associate the request with the private key
         */
        gnutls_x509_crq_set_key(crq, key);

        /* Self sign the certificate request.
         */
        gnutls_x509_crq_sign2(crq, key, GNUTLS_DIG_SHA1, 0);

        /* Export the PEM encoded certificate request, and
         * display it.
         */
        gnutls_x509_crq_export(crq, GNUTLS_X509_FMT_PEM, buffer,
                               &buffer_size);

        printf("Certificate Request: \n%s", buffer);


        /* Export the PEM encoded private key, and
         * display it.
         */
        buffer_size = sizeof(buffer);
        gnutls_x509_privkey_export(key, GNUTLS_X509_FMT_PEM, buffer,
                                   &buffer_size);

        printf("\n\nPrivate key: \n%s", buffer);

        gnutls_x509_crq_deinit(crq);
        gnutls_x509_privkey_deinit(key);

        return 0;

}
