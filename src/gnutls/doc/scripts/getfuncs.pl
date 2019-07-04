#!/usr/bin/perl

# Copyright (C) 2011-2012 Free Software Foundation, Inc.
# Copyright (C) 2013 Nikos Mavrogiannopoulos
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

# given a header file in stdin it will print all functions

my $line;
my $func_name;
my $prototype;

$state = 0;

# 0: scanning
# 1: comment
# 2: struct||enum||typedef func
# 3: function
# 4: inline function { }

sub function_print {
  my $prototype = shift @_;

  if ($prototype =~ m/^\s*\w\s+[A-Za-z0-9_]+[\s\*]+([A-Za-z0-9_]+)\s*\(.*/) {
    $func_name = $1;
  } elsif ($prototype =~ m/^\s*[A-Za-z0-9_]+[\s\*]+([A-Za-z0-9_]+)\s*\([^\)]+/) {
    $func_name = $1;
  } elsif ($prototype =~ m/^\s*\w+\s+\w+[\s\*]+([A-Za-z0-9_]+)\s*\([^\)]+/) {
    $func_name = $1;
  } elsif ($prototype =~ m/^[\s\*]*([A-Za-z0-9_]+)\s*\([^\)]+/) {
    $func_name = $1;
  } elsif ($prototype =~ m/^[\s\*]*[A-Za-z0-9_]+\s+([A-Za-z0-9_]+)/) {
    $func_name = $1;
  }

#print STDERR "function: $prototype\n";
  if ($func_name ne '' && ($func_name =~ m/^gnutls_.*/ || $func_name =~ m/dane_.*/ || $func_name =~ m/xssl_.*/)) {
    print $func_name . "\n";
  }
      
  return;
}

while ($line=<STDIN>) {

  next if ($line eq '');
# print STDERR "line($state): $line";

  #skip comments
  if ($state == 0) {
    if ($line =~ m/^\s*\/\*/) {

      next if ($line =~ m/\*\//);

      $state = 1;
      next;
    } elsif ($line =~ m/^\s*typedef\s+enum/ || $line =~ m/^\s*enum/ || 
             $line =~ m/^\s*struct/ || $line =~ m/^\s*typedef\s+struct/ ||
             $line =~ m/^\s*typedef/) {

      next if ($line =~ m/;/);
      if ($line =~ m/\{/) {
        $state = 2;
      } else {
        $state = 6;
      }
      next;
    } elsif ($line =~ m/^\s*extern\s+"C"/) {
      next;
    } elsif ($line =~ m/^\s*\{/) {
      next if ($line =~ m/\}/);
      $state = 4;
      next;
    } elsif ($line =~ m/^\s*#define/) {
      next if ($line !~ m/\\$/);
      $state = 5;
      next;
    } elsif ($line !~ m/^\s*extern/ && $line !~ m/^\s*typedef/ && $line !~ m/doc-skip/ && $line =~ m/^\s*\w/) {
      $state = 3;

      $prototype = "$line";
      $func_name = '';

      if ($line =~ m/;/) {
        function_print($prototype);
        $state = 0;
        next;
      }
    }
  } elsif ($state == 1) { # comment
    if ($line =~ m/\*\//) {
      $state = 0;
      next;
    }
  } elsif ($state == 2) { #struct||enum||typedef struct
    if ($line =~ m/\}/) {
      $state = 0;
      next;
    }
  } elsif ($state == 3) { #possible function
    $prototype .= $line;
    
    if ($line =~ m/;/) {
      $state = 0;

      function_print($prototype);
    }
  } elsif ($state == 4) { #inline function to be skipped
    if ($line =~ m/\}/) {
      $state = 0;
      next;
    }
  } elsif ($state == 5) { # define
    if ($line !~ m/\\$/) {
      $state = 0;
      next;
    }
  } elsif ($state == 6) { #typedef (not struct)
    if ($line =~ m/;/) {
      $state = 0;
      next;
    }
  }

}
