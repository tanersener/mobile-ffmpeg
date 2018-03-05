This directory contains files used by tests/cert.c.

These are interesting certificates, that used to cause a crash
or other artifacts during loading. If a filename.err is present
it should contain the expected error code from gnutls_x509_crt_import();
otherwise success (0) is assumed.
