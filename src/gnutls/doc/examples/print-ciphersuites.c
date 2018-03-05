/* This example code is placed in the public domain. */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

static void print_cipher_suite_list(const char *priorities)
{
        size_t i;
        int ret;
        unsigned int idx;
        const char *name;
        const char *err;
        unsigned char id[2];
        gnutls_protocol_t version;
        gnutls_priority_t pcache;

        if (priorities != NULL) {
                printf("Cipher suites for %s\n", priorities);

                ret = gnutls_priority_init(&pcache, priorities, &err);
                if (ret < 0) {
                        fprintf(stderr, "Syntax error at: %s\n", err);
                        exit(1);
                }

                for (i = 0;; i++) {
                        ret =
                            gnutls_priority_get_cipher_suite_index(pcache,
                                                                   i,
                                                                   &idx);
                        if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
                                break;
                        if (ret == GNUTLS_E_UNKNOWN_CIPHER_SUITE)
                                continue;

                        name =
                            gnutls_cipher_suite_info(idx, id, NULL, NULL,
                                                     NULL, &version);

                        if (name != NULL)
                                printf("%-50s\t0x%02x, 0x%02x\t%s\n",
                                       name, (unsigned char) id[0],
                                       (unsigned char) id[1],
                                       gnutls_protocol_get_name(version));
                }

                return;
        }
}

int main(int argc, char **argv)
{
        if (argc > 1)
                print_cipher_suite_list(argv[1]);
        return 0;
}
