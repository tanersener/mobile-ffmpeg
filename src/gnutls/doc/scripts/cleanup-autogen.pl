#!/usr/bin/perl

# Copyright (C) 2012 Free Software Foundation, Inc.
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

my $line;
my $init = 0;
my $menu = 0;
my $i = 0;

while ($line = <>) {

  if ($line =~ /\@node (.*)/) {
    if ($init == 0) {
      $init = 1;
    } else {
      print "\@anchor\{$1\}\n";
      next;
    }
  }

  if ($line =~ /\@menu/) {
    $menu = 1;
    next;
  }

  if ($line =~ /\@end menu/) {
    $menu = 0;
    next;
  }

  next if ($menu != 0);

  if ($line =~ /\@subsection\s(.*)/) {
    $line = "\@subheading $1\n";
  }

  if ($line =~ /\@cindex\s(.*)/) {
    if ($line !~ /help/) {
      next;
    }
  }

  if ($line =~ /^Please send bug reports.*/) {
    next;
  }

  print $line;
  $i++;
}

if ($i < 2) {
  exit 1;
}

exit 0;
