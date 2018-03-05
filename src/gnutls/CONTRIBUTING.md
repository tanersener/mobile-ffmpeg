# GnuTLS -- Information about our contribution rules and coding style

 Anyone is welcome to contribute to GnuTLS. You can either take up
tasks from our [planned list](https://gitlab.com/gnutls/gnutls/milestones),
or suprise us with enhancement we didn't plan for. In all cases be prepared
to defend and justify your enhancements, and get through few rounds
of changes. 

We try to stick to the following rules, so when contributing please
try to follow them too.

# Git commits:

  The original author of the changes should be the author of the commit.
The "Signed-off-by" git flag is used to by committers to indicate they
agreed with project's DCO as described in [DCO.txt](doc/DCO.txt). Note 
that we no longer require FSF copyright assignment.

# Test suite:

   New functionality should be accompanied by a test case which verifies
the correctness of GnuTLS' operation on successful use of the new
functionality, as well as on fail cases. The GnuTLS test suite is run on "make check"
on every system GnuTLS is installed, except for the tests/suite part
which is only run during development.

For testing functionality of gnutls we use two test unit testing frameworks:
1. The gnutls testing framework as in [utils.h](tests/utils.h), usually for high level
   tests such as testing a client against a server. See [set_x509_key_mem.c](tests/set_x509_key_mem.c).
2. The cmocka unit testing framework, for unit testing of functions
   or interfaces. See [dtls-sliding-window.c](tests/dtls-sliding-window.c).

Certificates for testing purposes are available at [cert-common.h](tests/cert-common.h).
Note that we do not regenerate test certificates when they expire, but
we rather fix the test's time using datefudge or gnutls_global_set_time_function().
For example, see [x509cert-tl.c](tests/x509cert-tl.c).

# File names:

  Files are split to directories according to the subsystem
they belong to. Examples are x509/, minitasn1/, openpgp/,
opencdk/ etc. The files in the root directory related
to the main TLS protocol implementation.


# C dialect:

  While parts of GnuTLS were written in older dialects, new code
in GnuTLS are expected to conform to C99. Exceptions could be made
for C99 features that are not supported in popular platforms on a
case by case basis.


# Indentation style:

 In general, use the Linux kernel coding style.  You may indent the source
using GNU indent, e.g. "indent -linux *.c".


# Function names:

  All the function names use underscore ```_```, to separate words,
functions like ```gnutlsDoThat``` are not used. The exported function names
usually start with the ```gnutls_``` prefix, and the words that follow
specify the exact subsystem of gnutls that this function refers to.
E.g. ```gnutls_x509_crt_get_dn```, refers to the X.509
certificate parsing part of gnutls. Some of the used prefixes are the
following.
 * ```gnutls_x509_crt_``` for the X.509 certificate part
 * ```gnutls_openpgp_key_``` for the openpgp key part
 * ```gnutls_session_``` for the TLS session part (but this may be omited)
 * ```gnutls_handshake_``` for the TLS handshake part
 * ```gnutls_record_``` for the TLS record protocol part
 * ```gnutls_alert_``` for the TLS alert protocol part
 * ```gnutls_credentials_``` for the credentials structures
 * ```gnutls_global_``` for the global structures handling

Internal functions -- that are not exported in the API -- should
be prefixed with an underscore. E.g. ```_gnutls_handshake_begin()```

Internal structures should not be exported. Especially pointers to
internal data. Doing so harms future reorganization/rewrite of subsystems.

All exported functions must be listed in libgnutls.map.in,
in order to be exported.


# Constructed types:

  The constructed types in gnutls always have the ```gnutls_``` prefix.
Definitions, value defaults and enumerated values should be in
capitals. E.g. ```GNUTLS_CIPHER_3DES_CBC```.

Structures should have the ```_st``` suffix in their name even
if they are a typedef. One can use the sizeof() on types with 
```_st``` as suffix to get the structure's size.

Other constructed types should have the ```_t``` suffix. A pointer
to a structure also has the ```_t``` suffix.


# Function parameters:

The gnutls functions accept parameters in the order:
 1. Input parameters
 2. Output parameters

When data and size is expected, a gnutls_datum structure should be
used (or more precisely a pointer to the structure).


# Callback function parameters:

 Callback functions should be avoided, if this is possible. 
Callbacks that refer to a TLS session should include the
current session as a parameter, in order for the called function to
be able to retrieve the data associated with the session.
This is not always done though -- see the push/pull callbacks.


# Return values:

 Functions in gnutls return an int type, when possible. In that
case 0 should be returned in case of success, or maybe a positive
value, if some other indication is needed.

A negative value always indicates failure. All the available
error codes are defined in gnutls.h and a description
is available in gnutls_errors.c


# Guile bindings:

 Parts of the Guile bindings, such as types (aka. "SMOBs"), enum values,
constants, are automatically generated.  This is handled by the modules
under `guile/modules/gnutls/build/'; these modules are only used at
build-time and are not installed.

The Scheme variables they generate (e.g., constants, type predicates,
etc.) are exported to user programs through `gnutls.scm' and
`gnutls/extra.scm', both of which are installed.

For instance, when adding/removing/renaming enumerates or constants,
two things must be done:

 1. Update the enum list in `build/enums.scm' (currently dependencies
    are not tracked, so you have to run "make clean all" in `guile/'
    after).

 2. Update the export list of `gnutls.scm' (or `extra.scm').

Note that, for constants and enums, "schemefied" names are used, as
noted under the "Guile API Conventions" node of the manual.
