#!/usr/bin/perl

# Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

$dir = shift;
$param2 = shift;

if (defined $param2 && $param2 ne '') {
  $enum = 1;
} else {
  $enum = 0;
}

sub key_of_record {
  local($record) = @_;

  # Split record into lines:
  my @lines = split /\n/, $record;

  my ($i) = 1;
  my ($key) = '';
  $key = $lines[$i] if (defined $lines[$i]);

  if ($enum == 1) {
    while( !($key =~ m/\@c\s(.*)\n/) && ($i < 5)) { $i=$i+1; $key = $lines[$i]; }
  } else {
    while( !($key =~ m/\@subheading\s(.*)/) && ($i < 5)) { 
      $i=$i+1; 
      if (defined $lines[$i]) {
        $key = $lines[$i]; 
      } else {
        $key = '';
      }
    }
  }

  return $key;
}

if ($enum == 1) {
  $/="\@end table";
} else {
  $/="\@end deftypefun";
}
@records = <>;  # Read in whole file, one record per array element.

$/="\n";

mkdir $dir;

if ($enum == 0) {
  @records = sort { key_of_record($a) cmp key_of_record($b) } @records;
}

foreach (@records) {
  $key = $_;
  if ($enum == 1) {
    $key =~ m/\@c\s(.*)\n/;
    $key = $1;
  } else {
    $key =~ m/\@subheading\s(.*)\n/;
    $key = $1;
  }

  if (defined $key && $key ne "") {
    open FILE, "> $dir/$key\n" or die $!;
    print FILE $_ . "\n";
    close FILE;
  }
} 

#print @records;
