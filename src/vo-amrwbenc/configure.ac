dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_INIT([vo-amrwbenc], [0.1.3], [http://sourceforge.net/projects/opencore-amr/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE

dnl Various options for configure
AC_ARG_ENABLE([armv5e],
            [AS_HELP_STRING([--enable-armv5e],
               [enable ARMV5E assembler (default is no)])],
            [armv5e=$enableval], [armv5e=no])
AC_ARG_ENABLE([armv7neon],
            [AS_HELP_STRING([--enable-armv7neon],
               [enable ARMV7 neon assembler (default is no)])],
            [armv7neon=$enableval], [armv7neon=no])
AC_ARG_ENABLE([example],
            [AS_HELP_STRING([--enable-example],
                [enable example encoding program (default is no)])],
            [example=$enableval], [example=no])

dnl Automake conditionals to set
AM_CONDITIONAL(ARMV5E, test x$armv5e = xyes)
AM_CONDITIONAL(ARMV7NEON, test x$armv7neon = xyes)
AM_CONDITIONAL(EXAMPLE, test x$example = xyes)

dnl Checks for programs.
AM_PROG_AS
AC_PROG_CC_C99

dnl Setup for libtool
LT_INIT

dnl soname version to use
dnl goes by ‘current[:revision[:age]]’ with the soname ending up as
dnl current.age.revision.
VO_AMRWBENC_VERSION=0:4:0
AC_SUBST(VO_AMRWBENC_VERSION)

AC_CONFIG_FILES([Makefile
                 vo-amrwbenc.pc])
AC_OUTPUT
