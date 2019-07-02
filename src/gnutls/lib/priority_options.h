/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf --global-table -t priority_options.gperf  */
/* Computed positions: -k'1,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "priority_options.gperf"

typedef void (*option_set_func)(gnutls_priority_t);
#line 6 "priority_options.gperf"
struct priority_options_st { const char *name; option_set_func func; };

#define TOTAL_KEYWORDS 34
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 32
#define MIN_HASH_VALUE 6
#define MAX_HASH_VALUE 52
/* maximum key range = 47, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 25,
      30, 53, 53,  5, 53, 53,  5, 53, 53, 53,
      53, 53, 53, 53, 53,  5, 53, 35,  0, 10,
       0, 30,  0, 53, 53,  0, 15,  0,  0, 53,
       0, 53, 53, 10,  5,  5,  0,  5, 53, 25,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
      53, 53, 53, 53, 53, 53
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

static const struct priority_options_st wordlist[] =
  {
    {""}, {""}, {""}, {""}, {""}, {""},
#line 12 "priority_options.gperf"
    {"NO_ETM", enable_no_etm},
    {""}, {""},
#line 13 "priority_options.gperf"
    {"FORCE_ETM", enable_force_etm},
    {""},
#line 9 "priority_options.gperf"
    {"DUMBFW", enable_dumbfw},
#line 34 "priority_options.gperf"
    {"PROFILE_HIGH", enable_profile_high},
#line 27 "priority_options.gperf"
    {"FALLBACK_SCSV", enable_fallback_scsv},
#line 33 "priority_options.gperf"
    {"PROFILE_MEDIUM", enable_profile_medium},
#line 14 "priority_options.gperf"
    {"NO_SESSION_HASH", enable_no_ext_master_secret},
#line 31 "priority_options.gperf"
    {"PROFILE_LOW", enable_profile_low},
#line 30 "priority_options.gperf"
    {"PROFILE_VERY_WEAK", enable_profile_very_weak},
#line 35 "priority_options.gperf"
    {"PROFILE_ULTRA", enable_profile_ultra},
#line 16 "priority_options.gperf"
    {"VERIFY_ALLOW_BROKEN", enable_verify_allow_broken},
#line 11 "priority_options.gperf"
    {"NO_TICKETS", enable_no_tickets},
#line 25 "priority_options.gperf"
    {"PARTIAL_RENEGOTIATION", enable_partial_safe_renegotiation},
#line 37 "priority_options.gperf"
    {"PROFILE_SUITEB128", enable_profile_suiteb128},
#line 10 "priority_options.gperf"
    {"NO_EXTENSIONS", enable_no_extensions},
#line 36 "priority_options.gperf"
    {"PROFILE_FUTURE", enable_profile_future},
#line 23 "priority_options.gperf"
    {"UNSAFE_RENEGOTIATION", enable_unsafe_renegotiation},
#line 26 "priority_options.gperf"
    {"DISABLE_SAFE_RENEGOTIATION", disable_safe_renegotiation},
#line 28 "priority_options.gperf"
    {"DISABLE_WILDCARDS", disable_wildcards},
#line 24 "priority_options.gperf"
    {"SAFE_RENEGOTIATION", enable_safe_renegotiation},
#line 20 "priority_options.gperf"
    {"SSL3_RECORD_VERSION", enable_ssl3_record_version},
#line 17 "priority_options.gperf"
    {"VERIFY_ALLOW_SIGN_RSA_MD5", enable_verify_allow_rsa_md5},
#line 15 "priority_options.gperf"
    {"STATELESS_COMPRESSION", dummy_func},
#line 22 "priority_options.gperf"
    {"VERIFY_ALLOW_X509_V1_CA_CRT", dummy_func},
    {""},
#line 41 "priority_options.gperf"
    {"ALLOW_SMALL_RECORDS", enable_allow_small_records},
#line 19 "priority_options.gperf"
    {"VERIFY_DISABLE_CRL_CHECKS", disable_crl_checks},
#line 21 "priority_options.gperf"
    {"LATEST_RECORD_VERSION", enable_latest_record_version},
#line 29 "priority_options.gperf"
    {"SERVER_PRECEDENCE", enable_server_precedence},
    {""},
#line 32 "priority_options.gperf"
    {"PROFILE_LEGACY", enable_profile_legacy},
    {""},
#line 39 "priority_options.gperf"
    {"NEW_PADDING", dummy_func},
#line 40 "priority_options.gperf"
    {"DEBUG_ALLOW_KEY_USAGE_VIOLATIONS", enable_server_key_usage_violations},
    {""}, {""}, {""},
#line 8 "priority_options.gperf"
    {"COMPAT", enable_compat},
#line 38 "priority_options.gperf"
    {"PROFILE_SUITEB192", enable_profile_suiteb192},
    {""}, {""}, {""}, {""},
#line 18 "priority_options.gperf"
    {"VERIFY_ALLOW_SIGN_WITH_SHA1", enable_verify_allow_sha1}
  };

static const struct priority_options_st *
in_word_set (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
