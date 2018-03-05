/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/pkcs12.h>

#include "examples.h"

#define OUTFILE "out.p12"

/* This function will write a pkcs12 structure into a file.
 * cert: is a DER encoded certificate
 * pkcs8_key: is a PKCS #8 encrypted key (note that this must be
 *  encrypted using a PKCS #12 cipher, or some browsers will crash)
 * password: is the password used to encrypt the PKCS #12 packet.
 */
int
write_pkcs12(const gnutls_datum_t * cert,
             const gnutls_datum_t * pkcs8_key, const char *password)
{
        gnutls_pkcs12_t pkcs12;
        int ret, bag_index;
        gnutls_pkcs12_bag_t bag, key_bag;
        char pkcs12_struct[10 * 1024];
        size_t pkcs12_struct_size;
        FILE *fd;

        /* A good idea might be to use gnutls_x509_privkey_get_key_id()
         * to obtain a unique ID.
         */
        gnutls_datum_t key_id = { (void *) "\x00\x00\x07", 3 };

        gnutls_global_init();

        /* Firstly we create two helper bags, which hold the certificate,
         * and the (encrypted) key.
         */

        gnutls_pkcs12_bag_init(&bag);
        gnutls_pkcs12_bag_init(&key_bag);

        ret =
            gnutls_pkcs12_bag_set_data(bag, GNUTLS_BAG_CERTIFICATE, cert);
        if (ret < 0) {
                fprintf(stderr, "ret: %s\n", gnutls_strerror(ret));
                return 1;
        }

        /* ret now holds the bag's index.
         */
        bag_index = ret;

        /* Associate a friendly name with the given certificate. Used
         * by browsers.
         */
        gnutls_pkcs12_bag_set_friendly_name(bag, bag_index, "My name");

        /* Associate the certificate with the key using a unique key
         * ID.
         */
        gnutls_pkcs12_bag_set_key_id(bag, bag_index, &key_id);

        /* use weak encryption for the certificate. 
         */
        gnutls_pkcs12_bag_encrypt(bag, password,
                                  GNUTLS_PKCS_USE_PKCS12_RC2_40);

        /* Now the key.
         */

        ret = gnutls_pkcs12_bag_set_data(key_bag,
                                         GNUTLS_BAG_PKCS8_ENCRYPTED_KEY,
                                         pkcs8_key);
        if (ret < 0) {
                fprintf(stderr, "ret: %s\n", gnutls_strerror(ret));
                return 1;
        }

        /* Note that since the PKCS #8 key is already encrypted we don't
         * bother encrypting that bag.
         */
        bag_index = ret;

        gnutls_pkcs12_bag_set_friendly_name(key_bag, bag_index, "My name");

        gnutls_pkcs12_bag_set_key_id(key_bag, bag_index, &key_id);


        /* The bags were filled. Now create the PKCS #12 structure.
         */
        gnutls_pkcs12_init(&pkcs12);

        /* Insert the two bags in the PKCS #12 structure.
         */

        gnutls_pkcs12_set_bag(pkcs12, bag);
        gnutls_pkcs12_set_bag(pkcs12, key_bag);


        /* Generate a message authentication code for the PKCS #12
         * structure.
         */
        gnutls_pkcs12_generate_mac(pkcs12, password);

        pkcs12_struct_size = sizeof(pkcs12_struct);
        ret =
            gnutls_pkcs12_export(pkcs12, GNUTLS_X509_FMT_DER,
                                 pkcs12_struct, &pkcs12_struct_size);
        if (ret < 0) {
                fprintf(stderr, "ret: %s\n", gnutls_strerror(ret));
                return 1;
        }

        fd = fopen(OUTFILE, "w");
        if (fd == NULL) {
                fprintf(stderr, "cannot open file\n");
                return 1;
        }
        fwrite(pkcs12_struct, 1, pkcs12_struct_size, fd);
        fclose(fd);

        gnutls_pkcs12_bag_deinit(bag);
        gnutls_pkcs12_bag_deinit(key_bag);
        gnutls_pkcs12_deinit(pkcs12);

        return 0;
}
