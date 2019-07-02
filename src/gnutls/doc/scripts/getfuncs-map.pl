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

# This list contains exported values, but prototype isn't easy
# to obtain (e.g., it's a pointer to a function or not a function).
my %known_false_positives = (
	'gnutls_srp_1024_group_generator' => 1,
	'gnutls_srp_1024_group_prime' => 1,
	'gnutls_srp_1536_group_generator' => 1,
	'gnutls_srp_1536_group_prime' => 1,
	'gnutls_srp_2048_group_generator' => 1,
	'gnutls_srp_2048_group_prime' => 1,
	'gnutls_srp_3072_group_generator' => 1,
	'gnutls_srp_3072_group_prime' => 1,
	'gnutls_srp_4096_group_generator' => 1,
	'gnutls_srp_4096_group_prime' => 1,
	'gnutls_srp_8192_group_generator' => 1,
	'gnutls_srp_8192_group_prime' => 1,
	'gnutls_ffdhe_2048_group_generator' => 1,
	'gnutls_ffdhe_2048_group_prime' => 1,
	'gnutls_ffdhe_2048_group_q' => 1,
	'gnutls_ffdhe_2048_key_bits' => 1,
	'gnutls_ffdhe_3072_group_generator' => 1,
	'gnutls_ffdhe_3072_group_prime' => 1,
	'gnutls_ffdhe_3072_group_q' => 1,
	'gnutls_ffdhe_3072_key_bits' => 1,
	'gnutls_ffdhe_4096_group_generator' => 1,
	'gnutls_ffdhe_4096_group_prime' => 1,
	'gnutls_ffdhe_4096_group_q' => 1,
	'gnutls_ffdhe_4096_key_bits' => 1,
	'gnutls_ffdhe_6144_group_generator' => 1,
	'gnutls_ffdhe_6144_group_prime' => 1,
	'gnutls_ffdhe_6144_group_q' => 1,
	'gnutls_ffdhe_6144_key_bits' => 1,
	'gnutls_ffdhe_8192_group_generator' => 1,
	'gnutls_ffdhe_8192_group_prime' => 1,
	'gnutls_ffdhe_8192_group_q' => 1,
	'gnutls_ffdhe_8192_key_bits' => 1,
	'gnutls_transport_set_int' => 1,
	'gnutls_strdup' => 1,
	'gnutls_realloc' => 1,
	'gnutls_free' => 1,
	'gnutls_malloc' => 1,
	'gnutls_calloc' => 1,
	'gnutls_secure_malloc' => 1,
	'gnutls_realloc_fast' => 1,
	'gnutls_secure_calloc' => 1,
	'gnutls_pubkey_get_pk_dsa_raw' => 1,
	'gnutls_pubkey_get_pk_ecc_raw' => 1,
	'gnutls_pubkey_get_pk_ecc_x962' => 1,
	'gnutls_pubkey_get_pk_rsa_raw' => 1,
);

# API functions that although documented as such, are simply
# macros that expand to another function.
my %known_false_negatives = (
	'gnutls_pkcs11_copy_x509_crt' => 1,
	'gnutls_pkcs11_copy_x509_privkey' => 1,
	'gnutls_pkcs11_privkey_generate' => 1,
	'gnutls_pkcs11_privkey_generate2' => 1,
	'gnutls_privkey_import_pkcs11_url' => 1,
	'gnutls_transport_set_int' => 1,
);

sub function_print {
  my $func_name;
  my $prototype = shift @_;
  my $check;

#print STDERR $prototype;
  if ($prototype =~ m/^\s*([A-Za-z0-9_]+)\;$/) {
#  if ($prototype =~ m/^(.*)/) {
    $func_name = $1;
  } else { 
    $func_name = '';
  }

  $check = $known_false_positives{$func_name};
  return if (defined $check && $check == 1);

  if ($func_name ne '' && ($func_name =~ m/^gnutls_.*/ || $func_name =~ m/^dane_.*/)) {
    print $func_name . "\n";
  }
      
  return;
}

my $line;
my $lineno = 0;
while ($line=<STDIN>) {

  next if ($line eq '');
# print STDERR "line($lineno): $line";

  #skip comments
#  if ($line =~ m/^\s*/) {
     function_print($line);
#  }
   $lineno++;
}

for (keys %known_false_negatives) {
	print $_ . "\n";
}
