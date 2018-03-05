GnuTLS README -- Important introductory notes
=============================================

GnuTLS implements the TLS/SSL (Transport Layer Security aka Secure
Sockets Layer) protocol.  GnuTLS is a GNU project.  Additional
information can be found at <http://www.gnutls.org/>.


README
======

This README is targeted for users of the library who build from
sources but do not necessarily develop.  If you are interested
in developing and contributing to the GnuTLS project, please
see README-alpha and visit
http://www.gnutls.org/manual/html_node/Contributing.html.


COMPILATION
===========

A typical command sequence for building the library is shown below.
A complete list of options available for configure can be found
by running './configure --help'.

```
    cd gnutls-<version>
    ./configure --prefix=/usr
    make
    make check
    sudo make install
```

The commands above build and install the static archive (libgnutls.a),
the shared object (libgnutls.so), and additional binaries such as certtool 
and gnutls-cli.

The library depends on libnettle and gmplib. 
* gmplib: for big number arithmetic, http://gmplib.org/
* nettle: for cryptographic algorithms, http://www.lysator.liu.se/~nisse/nettle/

Optionally it may use the following libraries:
* libtasn1: For ASN.1 parsing (a copy is included, if not found), http://www.gnu.org/software/libtasn1/
* p11-kit: for smart card support, http://p11-glue.freedesktop.org/p11-kit.html
* libtspi: for Trusted Platform Module (TPM) support, http://trousers.sourceforge.net/
* libunbound: For DNSSEC/DANE support, http://unbound.net/
* libz: For compression support, http://www.zlib.net/
* libidn: For supporting internationalized DNS names (IDNA 2003), http://www.gnu.org/software/libidn/
* libidn2: For supporting internationalized DNS names (IDNA 2008), https://www.gnu.org/software/libidn/#libidn2

To configure libnettle for installation and use by GnuTLS, a typical
command sequence would be:

```
    cd nettle-<version>
    ./configure --prefix=/usr --disable-openssl --enable-shared
    make
    sudo make install
```

For the Nettle project, --enable-shared will instruct automake and
friends to build and install both the static archive (libnettle.a)
and the shared object (libnettle.so).

In case you are compiling for an embedded system, you can disable
unneeded features of GnuTLS.  In general, it is usually best not to
disable anything (for future mailing list questions and possible bugs).

Depending on your installation, additional libraries, such as libtasn1
and zlib, may be required.


DOCUMENTATION
=============

See the documentation in doc/ and online at
http://www.gnutls.org/manual.


EXAMPLES
========

See the examples in doc/examples/ and online at 'How To Use GnuTLS in
Applications' at http://www.gnutls.org/manual.


SECURITY ADVISORIES
===================

The project collects and publishes information on past security
incidents and vulnerabilities.  Open information exchange, including
information which is [sometimes] suppressed in non-open or non-free
projects, is one of the goals of the GnuTLS project.  Please visit
http://www.gnutls.org/security.html.


MAILING LISTS
=============

The GnuTLS project maintains mailing lists for users, developers, and
commits.  Please see http://www.gnutls.org/lists.html.


LICENSING
=========

See the [LICENSE](LICENSE) file.


BUGS
====

Thorough testing is very important and expensive.  Often, the 
developers do not have access to a particular piece of hardware or 
configuration to reproduce a scenario.  Notifying the developers about a 
possible bug will greatly help the project.  

If you believe you have found a bug, please report it to bugs@gnutls.org
together with any applicable information. 

Applicable information would include why the issue is a GnuTLS bug (if
not readily apparent), output from 'uname -a', the version of the library or
tool being used, a stack trace if available ('bt full' if under gdb or
valgrind output), and perhaps a network trace.  Vague queries or piecemeal 
messages are difficult to act upon and don't help the development effort.

Additional information can be found at the project's manual.


PATCHES
=======

Patches are welcome and encouraged. Patches can be submitted through the 
bug tracking system or the mailing list.  When submitting patches, please 
be sure to use sources from the git repository, and preferably from the 
master branch.  To create a patch for the project from a local git repository, 
please use the following commands. 'gnutls' should be the local directory 
of a previous git clone.

```
    cd gnutls
    git add the-file-you-modified.c another-file.c
    git commit the-file-you-modified.c another-file.c
    git format-patch
```

For more information on use of Git, visit http://git-scm.com/

----------------------------------------------------------------------
Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.
