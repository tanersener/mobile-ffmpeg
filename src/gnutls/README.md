[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/330/badge)](https://bestpractices.coreinfrastructure.org/projects/330)

|Branch|CI system|Status|Test suite coverage|Fuzzer coverage|
|:----:|:-------:|-----:|:------:|:-------------:|
|Master/3.6.x|Gitlab|[![build status](https://gitlab.com/gnutls/gnutls/badges/master/pipeline.svg)](https://gitlab.com/gnutls/gnutls/commits/master)|[![coverage report](https://gitlab.com/gnutls/gnutls/badges/master/coverage.svg)](https://gnutls.gitlab.io/coverage/master)|[![Fuzzer coverage report](https://gnutls.gitlab.io/coverage/master-fuzz/badge.svg)](https://gnutls.gitlab.io/coverage/master-fuzz)|
|Master/3.6.x|Travis|[![build status](https://travis-ci.org/gnutls/gnutls.svg?branch=master)](https://travis-ci.org/gnutls/gnutls)|N/A|N/A|


# GnuTLS -- Information for developers

GnuTLS implements the TLS/SSL (Transport Layer Security aka Secure
Sockets Layer) protocol.  Additional information can be found at
[www.gnutls.org](https://www.gnutls.org/).

This file contains instructions for developers and advanced users that
want to build from version controlled sources. See [INSTALL.md](INSTALL.md)
for building released versions.

We require several tools to check out and build the software, including:

* [Make](https://www.gnu.org/software/make/)
* [Automake](https://www.gnu.org/software/automake/) (use 1.11.3 or later)
* [Autoconf](https://www.gnu.org/software/autoconf/)
* [Autogen](https://www.gnu.org/software/autogen/) (use 5.16 or later)
* [Libtool](https://www.gnu.org/software/libtool/)
* [Gettext](https://www.gnu.org/software/gettext/)
* [Texinfo](https://www.gnu.org/software/texinfo/)
* [Tar](https://www.gnu.org/software/tar/)
* [Gzip](https://www.gnu.org/software/gzip/)
* [Texlive & epsf](https://www.tug.org/texlive/) (for PDF manual)
* [GTK-DOC](https://www.gtk.org/gtk-doc/) (for API manual)
* [Git](https://git-scm.com/)
* [Perl](https://www.cpan.org/)
* [Nettle](https://www.lysator.liu.se/~nisse/nettle/)
* [Guile](https://www.gnu.org/software/guile/)
* [p11-kit](https://p11-glue.github.io/p11-glue/p11-kit.html)
* [gperf](https://www.gnu.org/software/gperf/)
* [libtasn1](https://www.gnu.org/software/libtasn1/) (optional)
* [Libidn2](https://www.gnu.org/software/libidn/#libidn2) (optional, for internationalization of DNS, IDNA 2008)
* [Libunistring](https://www.gnu.org/software/libunistring/) (optional, for internationalization)
* [AWK](https://www.gnu.org/software/awk/) (for make dist, pmccabe2html)
* [bison](https://www.gnu.org/software/bison) (for datetime parser in certtool)
* [libunbound](https://unbound.net/) (for DANE support)
* [libabigail](https://pagure.io/libabigail/) (for abi comparison in make dist)
* [tcsd](https://trousers.sourceforge.net/) (for TPM support; optional)
* [swtpm](https://github.com/stefanberger/swtpm) (for TPM test; optional)
* [ncat](https://nmap.org/download.html) (for TPM test; optional)
* [tpm-tools](https://trousers.sourceforge.net/) (for TPM test; optional)
* [expect](https://core.tcl.tk/expect/index) (for TPM test; optional)

The required software is typically distributed with your operating
system, and the instructions for installing them differ.  Here are
some hints:

Debian/Ubuntu:
```
apt-get install -y dash git-core autoconf libtool gettext autopoint
apt-get install -y automake autogen nettle-dev libp11-kit-dev libtspi-dev libunistring-dev
apt-get install -y guile-2.2-dev libtasn1-6-dev libidn2-0-dev gawk gperf
apt-get install -y libunbound-dev dns-root-data bison gtk-doc-tools
apt-get install -y texinfo texlive texlive-generic-recommended texlive-extra-utils
```

__NOTE:__ Some software versions might not be available in older releases, e.g. `nettle-dev`. 
Available backport repos, APT-Pinning or source code compilating can be used to install these versions (and dependencies) from a newer release.

Fedora/RHEL:
```
yum install -y dash git autoconf libtool gettext-devel automake autogen patch
yum install -y nettle-devel p11-kit-devel autogen-libopts-devel libunistring-devel
yum install -y trousers-devel guile22-devel libtasn1-devel libidn2-devel gawk gperf
yum install -y libtasn1-tools unbound-devel bison gtk-doc texinfo texlive
```

Sometimes, you may need to install more recent versions of Automake,
Nettle, P11-kit and Autogen, which you will need to build from sources. 

Dependencies that are used during make check or make dist are listed below.
Moreover, for basic interoperability testing you may want to install openssl
and mbedtls.

* [Valgrind](https://valgrind.org/) (optional)
* [Libasan](https://gcc.gnu.org//) (optional)
* [datefudge](https://packages.debian.org/datefudge) (optional)
* [nodejs](https://nodejs.org/) (needed for certain test cases)
* [softhsm](https://www.opendnssec.org/softhsm/) (for testing smart card support)
* [dieharder](https://www.phy.duke.edu/~rgb/General/dieharder.php) (for testing PRNG)
* [lcov](https://linux-test-project.github.io/) (for code coverage)
* [util-linux](https://github.com/karelzak/util-linux) or just [lscpu](https://github.com/NanXiao/lscpu) (for CPU feature detection)
* [libev](hhttp://software.schmorp.de/pkg/libev.html) (for testing)

Debian/Ubuntu:
```
apt-get install -y valgrind nodejs softhsm2 datefudge lcov libssl-dev libcmocka-dev expect libev-dev
apt-get install -y dieharder openssl abigail-tools socat net-tools ppp lockfile-progs util-linux
```

__NOTE:__ `libubsan0` and `libasan1` are required on older versions of Ubuntu <= 16.04. This packages must be manually added on these versions:

```
apt-get install -y v libubsan0 libasan1
```

Fedora/RHEL:
```
yum install -y valgrind libasan libasan-static libubsan nodejs softhsm datefudge lcov openssl-devel expect libev-devel
yum install -y dieharder mbedtls-utils openssl libabigail libcmocka-devel socat lockfile-progs util-linux
```


To download the version controlled sources:

```
$ git clone https://gitlab.com/gnutls/gnutls.git
$ cd gnutls
```

The next step is to bootstrap and ./configure:

```
$ ./bootstrap
$ ./configure
```

When built this way, some developer defaults will be enabled.  See
cfg.mk for details.

Then build the project normally, and run the test suite.

```
$ make
$ make check
```

To test the code coverage of the test suite use the following:
```
$ ./configure --enable-code-coverage
$ make && make check && make code-coverage-capture
```

Individual tests that may require additional hardware (e.g., smart cards)
are:
```
$ sh tests/suite/testpkcs11
```

# Building for windows

It is recommended to cross compile using Fedora and the following
dependencies:

```
yum install -y wine mingw32-nettle mingw32-libtasn1 mingw32-gcc
```

and build as:

```
mingw32-configure --enable-local-libopts --disable-non-suiteb-curves --disable-doc --without-p11-kit
mingw32-make
mingw32-make check
```

# Continuous Integration (CI)

We utilize two continuous integration systems, the gitlab-ci and travis.
Gitlab-CI is used to test most of the Linux systems (see .gitlab-ci.yml),
and is split in two phases, build image creation and compilation/test. The
build image creation is done at the gnutls/build-images subproject and
uploads the image at the gitlab.com container registry. The compilation/test
phase is on every commit to gnutls project.

The Travis based CI, is used to test compilation on MacOSX based systems.


# Contributing

See [the contributing document](CONTRIBUTING.md).


Happy hacking!

----------------------------------------------------------------------
Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.
