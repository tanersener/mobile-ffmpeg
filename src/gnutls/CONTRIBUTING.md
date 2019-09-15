# GnuTLS -- Information about our contribution rules and coding style

 Anyone is welcome to contribute to GnuTLS. You can either take up
tasks from our [planned list](https://gitlab.com/gnutls/gnutls/milestones),
or suprise us with enhancement we didn't plan for. In all cases be prepared
to defend and justify your enhancements, and get through few rounds
of changes. 

We try to stick to the following rules, so when contributing please
try to follow them too.


# Git commits:

Note that when contributing code you will need to assert that the contribution is
in accordance to the "Developer's Certificate of Origin" as found in the 
file [DCO.txt](doc/DCO.txt).

To indicate that, make sure that your contributions (patches or merge requests),
contain a "Signed-off-by" line, with your real name and e-mail address. 
To automate the process use "git am -s" to produce patches and/or set the
a template to simplify this process, as follows.

```
$ cp devel/git-template ~/.git-template
[edit]
$ git config commit.template ~/.git-template
```


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

 In general, use [the Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html).
You may indent the source using GNU indent, e.g. "indent -linux *.c".


# Commenting style

In general for documenting new code we prefer self-documented code to comments. That is:
  - Meaningful function and macro names
  - Short functions which do a single thing

That does not mean that no comments are allowed, but that when they are
used, they are used to document something that is not obvious, or the protocol
expectations. Though we haven't followed that rule strictly in the past, it
should be followed on new code.


# Function names:

  All the function names use underscore ```_```, to separate words,
functions like ```gnutlsDoThat``` are not used. The exported function names
usually start with the ```gnutls_``` prefix, and the words that follow
specify the exact subsystem of gnutls that this function refers to.
E.g. ```gnutls_x509_crt_get_dn```, refers to the X.509
certificate parsing part of gnutls. Some of the used prefixes are the
following.
 * ```gnutls_x509_crt_``` for the X.509 certificate part
 * ```gnutls_session_``` for the TLS session part (but this may be omited)
 * ```gnutls_handshake_``` for the TLS handshake part
 * ```gnutls_record_``` for the TLS record protocol part
 * ```gnutls_alert_``` for the TLS alert protocol part
 * ```gnutls_credentials_``` for the credentials structures
 * ```gnutls_global_``` for the global structures handling

All exported API functions must be listed in libgnutls.map
in order to be exported.

Internal functions, i.e, functions that are not exported in the API but
are used internally by multiple files, should be prefixed with an underscore.
For example `_gnutls_handshake_begin()`.

Internal functions restricted to a file (static), or inline functions, should
not use the `_gnutls` prefix for simplicity, e.g., `get_version()`.

Internal structures should not be exported. Especially pointers to
internal data. Doing so harms future reorganization/rewrite of subsystems.
They can however be used by unit tests in tests/ directory; in that
case they should be part of the GNUTLS_PRIVATE_3_4 tag in libgnutls.map.


# Header guards

  Each private C header file SHOULD have a header guard consisting of the
project name and the file path relative to the project directory, all uppercase.

Example: `lib/srp.h` uses the header guard `GNUTLS_LIB_SRP_H`.

The header guard is used as first and last effective code in a header file,
like e.g. in lib/srp.h:

```
#ifndef GNUTLS_LIB_SRP_H
#define GNUTLS_LIB_SRP_H

...

#endif /* GNUTLS_LIB_SRP_H */
```

The public header files follow a similar convention but use the relative
install directory as template, e.g. `GNUTLS_GNUTLS_H` for `gnutls/gnutls.h`.


# Introducing new functions / API

  Prior to introducing any new API consider all options to offer the same
functionality without introducing a new function. The reason is that  we want
to avoid breaking the ABI, and thus we cannot typically remove any function
that was added (though we have few exceptions). Since we cannot remove, it
means that experimental APIs, or helper APIs that are not typically needed
may become a burden to maintain in the future. That is, they may prevent
a refactoring, or require to keep legacy code.

As such, some questions to answer before adding a new API:
 * Is this API useful for a large class of applications, or is it limited
   to few?
 * If it is limited to few, can we work around that functionality without
   a new API?
 * Would that function be relevant in the future when a new protocol such TLS
   13.0 is made available? Would it harm the addition of a new protocol?


The make rule 'abi-check' verifies that the ABI remained compatible since
the last tagged release. It relies on the git tree and abi-compliance-checker.

The above do not apply to the C++ library; this library's ABI should not
be considered stable.


# Introducing new features / modifying behavior

  When a new feature is introduced which may affect already deployed code, 
it must be disabled by default. For example a new TLS extension should be
enabled when explicitly requested by the application. That can happen for
example with a gnutls_init() flag.

The same should be followed when an existing function behavior is modified
in a way that may break existing applications which use the API in a
reasonable way. If the existing function allows flags, then a new flag
should be introduced to enable the new behavior.

When it is necessary, or desireable to enable the new features by default
(e.g., TLS1.3 introduction), the "next" releases should be used (and
introduced if necessary), to allow the modification to be tested for an
extended amount of time (see the [Release policy](RELEASES.md)).


# API documentation

When introducing a new API, we provide the function documentation as
inline comments, in a way that it can be used to generate a man-page
and be included in our manual. For that we use gnome-style comments
as in the example below. The detailed form is documented on `doc/scripts/gdoc`.

/**
 * gnutls_init:
 * @session: is a pointer to a #gnutls_session_t type.
 * @flags: indicate if this session is to be used for server or client.
 *
 * This function initializes the provided session. Every
 * session must be initialized before use, and must be deinitialized
 * after used by calling gnutls_deinit().
 *
 * @flags can be any combination of flags from %gnutls_init_flags_t.
 *
 * Note that since version 3.1.2 this function enables some common
 * TLS extensions such as session tickets and OCSP certificate status
 * request in client side by default. To prevent that use the %GNUTLS_NO_EXTENSIONS
 * flag.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or a negative error code.
 **/


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

When data and size are expected as input, a const gnutls_datum_t structure
should be used (or more precisely a pointer to the structure).

When data pointer and size are to be returned as output, a gnutls_datum_t
structure should be used.

When output is to be copied to caller an array of fixed data should be
provided.


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


Functions which are intended to return a boolean value should return
a type of bool, and it is recommended to contain the string '_is_'
on its function name; e.g.,
```
bool _gnutls_is_not_prehashed();
```

That allows the distinguishing functions that return negative errors
from boolean functions to both the developer and the compiler. Note
that in the past the 'unsigned' type was used to distinguish boolean functions
and several of these still exist.

## Selecting the right return value

When selecting the return value for a TLS protocol parsing function
a suggested approach is to check which alert fits best on that error
(see `alert.c`), and then select from the error codes which are mapped
to that alert (see `gnutls_error_to_alert()`). For more generic parsing
errors consider using the `GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER`.


# Usage of assert()

 The assert() macro --not to be confused with gnutls_assert()-- is used
exceptionally on impossible situations to assist static analysis tools.
That is, it should be used when the static analyzer used in CI (currently
clang analyzer), detects an error which is on an impossible situation.
In these cases assert() is used to rule out that case.

For example in the situation where a pointer is known to be non-null,
but the static analyzer cannot rule it out, we use code like the following:
```
assert(ptr != NULL);
ptr->deref = 3;
```

Since GnuTLS is a library no other uses of assert() macro are acceptable.

The NDEBUG macro is not used in GnuTLS compilation, so the assert() macros
are always active.


# Gnulib

The directories `gl/`, `src/gl/` and `lib/unistring` contain gnulib files
copied/created by `./bootstrap`. Gnulib is a portability source code library
to handle API or behavior incompatibilities between target systems.

To take advantage of the latest gnulib files, we have to update the
`gnulib/` submodule from time to time:
```
$ make glimport
```

Note that the gnulib library in `gl/` is used by the GnuTLS library
and is kept separate from the gnulib used by the GnuTLS tools because
of license issues, and also to prevent any gnulib networking modules
from entering the library (gnulib networking re-implements the windows
network stack and causes issues to gnutls applications running on windows).


# Compiler warnings

The compiler prints warnings for several reasons; these warnings are
also not constant in time, different versions of the same compiler may
warn about different issues.

In GnuTLS we enable as many as possible warnings available in the compiler
via configure.ac. On certain cases however we silence or disable warnings
and the following subsections go case by case.

## Switch unintended fall-through warnings

These we silence by using the macro FALLTHROUGH under a switch
statement which intentionally falls through. Example:
```
switch (session->internals.recv_state) {
        case RECV_STATE_DTLS_RETRANSMIT:
                ret = _dtls_retransmit(session);
                if (ret < 0)
                        return gnutls_assert_val(ret);

                session->internals.recv_state = RECV_STATE_0;
                FALLTHROUGH;
        case RECV_STATE_0:

                _dtls_async_timer_check(session);
                return 1;
}
```


# Symbol and library versioning

 The library uses the libtool versioning system, which in turn
results to a soname bump on incompatible changes. That is described
in [hooks.m4](m4/hooks.m4). Despite its complexity that system is
only sufficient to distinguish between versions of the library that
have broke ABI (i.e., soname bump occurred).

Today however, soname versioning isn't sufficient. Symbol versioning
as provided by [libgnutls.map](lib/libgnutls.map) have several
advantages.
 * they allow for symbol clashing between different gnutls library
   versions being in the same address space.
 * they allow installers to detect the library version used for
   an application utilizing a specific symbol
 * the allow introducing multiple versions of a symbol a la libc,
   keeping the semantics of old functions while introducing new.

As such for every symbol introduced on a particular version, we
create an entry in libgnutls.map based on the version and containing
the new symbols. For example, if in version 3.6.2 we introduce symbol
```gnutls_xyz```, the entry would be:

GNUTLS_3_6_2 {
  global:
	gnutls_xyz;
} GNUTLS_3_6_1;

where ```GNUTLS_3_6_1``` is the last version that symbols were introduced,
and indicates a dependency of 3.6.2 to symbols of 3.6.1.

Note that when the soname version is bumped, i.e., the ABI is broken
all the previous symbol versions should be combined into a single. For
example on the 3.4.0 soname bump, all symbols were put under the
GNUTLS_3_4 version.

Backporting new symbols to an old version which is soname compatible
is not allowed (can cause quite some problems). Backporting symbols
to an incompatible soname version is allowed, but must ensure that
the symbol version used for the backported symbol version is distinct from
the original library symbol version. E.g., if symbol ```gnutls_xyz```
with version GNUTLS_3_6_3 is backported on gnutls 3.3.15, it should
use version GNUTLS_3_3_15.


# Auto-generated files:
 Several parts of the documentation and the command line tools parameters
files (.def) are auto-generated. Normally when introducing new functions,
or adding new command line options to tools you need to run 'make
files-update', review the output (when feasible) and commit it separately,
e.g., with a message: "auto-generated files update".


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

# Automated testing

 GnuTLS primarily relies on gitlab-ci which is configured in .gitlab-ci.yml
file in the repository. The goal is to have a test suite which runs for
every new merge request prior to merging. There are no particular rules for
the test targets, except for them being reliable and running in a reasonable
timeframe (~1 hour).


# Reviewing code

 A review as part of the gitlab merge requests, is a way to prevent errors due to
these guidelines not being followed, e.g., verify there is a reasonable test suite,
and whether it covers reasonably the new code, that the function naming is
consistent with these guidelines, as well as check for obvious mistakes in the new
code.

The intention is to keep reviews lightweight, and rely on CI for tasks such
as compiling and testing code and features.

A proposed checklist to assist such reviews follows.
 * [ ] Any issues marked for closing are addressed
 * [ ] There is a test suite reasonably covering new functionality or modifications
 * [ ] Function naming, parameters, return values, types, etc., are consistent and according to `CONTRIBUTION.md`
 * [ ] This feature/change has adequate documentation added
 * [ ] No obvious mistakes in the code


[Guidelines to consider when reviewing.](https://github.com/thoughtbot/guides/tree/master/code-review)
