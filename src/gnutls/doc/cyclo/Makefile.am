## Process this file with automake to produce Makefile.in
# Copyright (C) 2008-2012 Free Software Foundation, Inc.
#
# Author: Simon Josefsson
#
# This file is part of GnuTLS.
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This file is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

EXTRA_DIST = cyclo-gnutls.html

vcurl = "http://git.savannah.gnu.org/gitweb/?p=gnutls.git;a=blob;f=lib/%FILENAME%;hb=HEAD"

cyclo-gnutls.html:
	(cd ${top_srcdir}/lib && \
		$(PMCCABE) `find . -type f -name \*.[ch] \
				| grep -v /minitasn1/ | sed 's,^./,,'` \
		| sort -nr \
		| LANG=C $(AWK) -f ${abs_top_srcdir}/build-aux/pmccabe2html \
			-v lang=html -v name="$(PACKAGE_STRING)" \
			-v vcurl=$(vcurl) \
			-v url="http://www.gnu.org/software/gnutls/" \
			-v css=${abs_top_srcdir}/build-aux/pmccabe.css) \
			> tmp
	mv tmp $@
