/* This example code is placed in the public domain. */

#include <config.h>
#include <gnutls/gnutls.h>
#include <gnutls/pkcs11.h>
#include <stdio.h>
#include <stdlib.h>

#define URL "pkcs11:URL"

int main(int argc, char **argv)
{
        gnutls_pkcs11_obj_t *obj_list;
        gnutls_x509_crt_t xcrt;
        unsigned int obj_list_size = 0;
        gnutls_datum_t cinfo;
        int ret;
        unsigned int i;

        ret = gnutls_pkcs11_obj_list_import_url4(&obj_list, &obj_list_size, URL,
                                                GNUTLS_PKCS11_OBJ_FLAG_CRT|
                                                GNUTLS_PKCS11_OBJ_FLAG_WITH_PRIVKEY);
        if (ret < 0)
                return -1;

        /* now all certificates are in obj_list */
        for (i = 0; i < obj_list_size; i++) {

                gnutls_x509_crt_init(&xcrt);

                gnutls_x509_crt_import_pkcs11(xcrt, obj_list[i]);

                gnutls_x509_crt_print(xcrt, GNUTLS_CRT_PRINT_FULL, &cinfo);

                fprintf(stdout, "cert[%d]:\n %s\n\n", i, cinfo.data);

                gnutls_free(cinfo.data);
                gnutls_x509_crt_deinit(xcrt);
        }

        for (i = 0; i < obj_list_size; i++)
                gnutls_pkcs11_obj_deinit(obj_list[i]);
	gnutls_free(obj_list);

        return 0;
}
