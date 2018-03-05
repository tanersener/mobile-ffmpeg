|Branch|CI system|Status|Coverage|
|:----:|:-------:|-----:|:------:|
|Master|Gitlab|[![build status](https://gitlab.com/gnutls/gnutls/badges/master/build.svg)](https://gitlab.com/gnutls/gnutls/commits/master)|[![coverage report](https://gitlab.com/gnutls/gnutls/badges/master/coverage.svg)](https://gitlab.com/gnutls/gnutls/commits/master)|
|Master|Travis|[![build status](https://travis-ci.org/gnutls/gnutls.svg?branch=master)](https://travis-ci.org/gnutls/gnutls)|N/A|
|3.4.x|Gitlab|[![build status](https://gitlab.com/gnutls/gnutls/badges/gnutls_3_4_x/build.svg)](https://gitlab.com/gnutls/gnutls/commits/gnutls_3_4_x)|N/A|
|3.3.x|Gitlab|[![build status](https://gitlab.com/gnutls/gnutls/badges/gnutls_3_3_x/build.svg)](https://gitlab.com/gnutls/gnutls/commits/gnutls_3_3_x)|N/A|


# GnuTLS -- Information for developers

GnuTLS implements the TLS/SSL (Transport Layer Security aka Secure
Sockets Layer) protocol.  GnuTLS is a GNU project.  Additional
information can be found at <http://www.gnutls.org/>.

This file contains instructions for developers and advanced users that
want to build from version controlled sources. See [INSTALL.md](INSTALL.md)
for building released versions.

We require several tools to check out and build the software, including:

* [Make](http://www.gnu.org/software/make/)
* [Automake](http://www.gnu.org/software/automake/) (use 1.11.3 or later)
* [Autoconf](http://www.gnu.org/software/autoconf/)
* [Autogen](http://www.gnu.org/software/autogen/) (use 5.16 or later)
* [Libtool](http://www.gnu.org/software/libtool/)
* [Gettext](http://www.gnu.org/software/gettext/)
* [Texinfo](http://www.gnu.org/software/texinfo/)
* [Tar](http://www.gnu.org/software/tar/)
* [Gzip](http://www.gnu.org/software/gzip/)
* [Texlive & epsf](http://www.tug.org/texlive/) (for PDF manual)
* [GTK-DOC](http://www.gtk.org/gtk-doc/) (for API manual)
* [Git](http://git-scm.com/)
* [Perl](http://www.cpan.org/)
* [Nettle](http://www.lysator.liu.se/~nisse/nettle/)
* [Guile](http://www.gnu.org/software/guile/)
* [p11-kit](http://p11-glue.freedesktop.org/p11-kit.html)
* [gperf](http://www.gnu.org/software/gperf/)
* [libtasn1](https://www.gnu.org/software/libtasn1/) (optional)
* [Libidn](http://www.gnu.org/software/libidn/) (optional, for internationalization of DNS, IDNA 2003)
* [Libidn2](https://www.gnu.org/software/libidn/#libidn2) (optional, for internationalization of DNS, IDNA 2008)
* [Libunistring](http://www.gnu.org/software/libunistring/) (optional, for internationalization)
* [AWK](http://www.gnu.org/software/awk/) (for make dist, pmccabe2html)
* [git2cl](http://savannah.nongnu.org/projects/git2cl/) (for make dist, ChangeLog)
* [bison](http://www.gnu.org/software/bison) (for datetime parser in certtool)
* [libunbound](https://unbound.net/) (for DANE support)
* [abi-compliance-checker](http://ispras.linuxbase.org/index.php/ABI_compliance_checker) (for make dist)

The required software is typically distributed with your operating
system, and the instructions for installing them differ.  Here are
some hints:

Debian/Ubuntu:
```
apt-get install -y git-core autoconf libtool gettext autopoint
apt-get install -y automake autogen nettle-dev libp11-kit-dev libtspi-dev libunistring-dev
apt-get install -y guile-2.0-dev libtasn1-6-dev libidn11-dev gawk gperf git2cl
apt-get install -y libunbound-dev dns-root-data bison help2man gtk-doc-tools
apt-get install -y texinfo texlive texlive-generic-recommended texlive-extra-utils
```

Fedora/RHEL:
```
yum install -y git autoconf libtool gettext-devel automake autogen
yum install -y nettle-devel p11-kit-devel autogen-libopts-devel libunistring-devel
yum install -y trousers-devel guile-devel libtasn1-devel libidn-devel gawk gperf git2cl
yum install -y libtasn1-tools unbound-devel bison help2man gtk-doc texinfo texlive
```

Sometimes, you may need to install more recent versions of Automake,
Nettle, P11-kit and Autogen, which you will need to build from sources. 

Dependencies that are used during make check or make dist are listed below.
Moreover, for basic interoperability testing you may want to install openssl
and polarssl.

* [Valgrind](http://valgrind.org/) (optional)
* [Libasan](https://gcc.gnu.org//) (optional)
* [datefudge](http://packages.debian.org/datefudge) (optional)
* [nodejs](http://nodejs.org/) (needed for certain test cases)
* [softhsm](http://www.opendnssec.org/softhsm/) (for testing smart card support)
* [dieharder](http://www.phy.duke.edu/~rgb/General/dieharder.php) (for testing PRNG)
* [lcov](http://linux-test-project.github.io/) (for code coverage)

Debian/Ubuntu:
```
apt-get install -y valgrind libasan1 libubsan0 nodejs softhsm2 datefudge lcov libssl-dev libcmocka-dev
apt-get install -y dieharder libpolarssl-runtime openssl abi-compliance-checker socat net-tools ppp
```

Fedora/RHEL:
```
yum install -y valgrind libasan libasan-static libubsan nodejs softhsm datefudge lcov openssl-devel
yum install -y dieharder mbedtls-utils openssl abi-compliance-checker libcmocka-devel socat
```


To download the version controlled sources:

```
$ git clone https://gitlab.com/gnutls/gnutls.git
$ cd gnutls
$ git submodule update --init
```

The next step is to run autoreconf (etc) and then ./configure:

```
$ make bootstrap
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

If you wish to contribute, you may read more about our [coding style](CONTRIBUTING.md).
Note that when contributing code that is not assigned to FSF, you will
need to assert that the contribution is in accordance to the "Developer's
Certificate of Origin" as found in the file [DCO.txt](doc/DCO.txt).
That can be done by sending a mail with your real name to the gnutls-devel
mailing list. Then just make sure that your contributions (patches),
contain a "Signed-off-by" line, with your name and e-mail address. 
To automate the process use "git am -s" to produce patches.

Happy hacking!

----------------------------------------------------------------------
Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.
