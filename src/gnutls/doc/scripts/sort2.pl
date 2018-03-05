#!/usr/bin/perl

# Copyright (C) 2004-2012 Free Software Foundation, Inc.
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

sub key_of_record {
  local($record) = @_;

  # Split record into lines:
  my @lines = split /\n/, $record;
  my $max = @lines;
  if ($max > 5) {
    $max = 5;
  }
  
  if ($max < 2) {
    return "";
  }

  my ($i) = 1;
  my ($key) = $lines[$i];

  while( !($key =~ /^\@deftypefun/) && ($i < $max)) { $i=$i+1; $key = $lines[$i]; }

  $key = $1 if $key =~ /^\@deftypefun {.*} {(.*)}/;

#  print STDERR "key $1\n";

  return $key;
}

$/="\@end deftypefun";          # Records are separated by blank lines.
@records = <>;  # Read in whole file, one record per array element.

@records = sort { key_of_record($a) cmp key_of_record($b) } @records;
print @records;
