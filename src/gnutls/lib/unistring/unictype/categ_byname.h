/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m 10 ../../../lib/unistring/unictype/categ_byname.gperf  */
/* Computed positions: -k'1-2,7,$' */

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

#line 2 "../../../lib/unistring/unictype/categ_byname.gperf"
struct named_category { int name; unsigned int category_index; };

#define TOTAL_KEYWORDS 103
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 21
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 150
/* maximum key range = 150, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_STRCMP
#define GPERF_CASE_STRCMP 1
static int
gperf_case_strcmp (register const char *s1, register const char *s2)
{
  for (;;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 != 0 && c1 == c2)
        continue;
      return (int)c1 - (int)c2;
    }
}
#endif

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
general_category_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151,   1, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151,  16, 151,   1,  66,  21,
        9,  25, 151,  62, 151,  49,   0,  51,   4,   7,
        6, 151,  25,  42,   5,   8, 151, 151, 151,   0,
       45, 151, 151, 151, 151, 151, 151,  16, 151,   1,
       66,  21,   9,  25, 151,  62, 151,  49,   0,  51,
        4,   7,   6, 151,  25,  42,   5,   8, 151, 151,
      151,   0,  45, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151, 151, 151, 151, 151,
      151, 151, 151, 151, 151, 151
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct general_category_stringpool_t
  {
    char general_category_stringpool_str1[sizeof("L")];
    char general_category_stringpool_str2[sizeof("Ll")];
    char general_category_stringpool_str3[sizeof("C")];
    char general_category_stringpool_str4[sizeof("LC")];
    char general_category_stringpool_str5[sizeof("Cc")];
    char general_category_stringpool_str6[sizeof("Nl")];
    char general_category_stringpool_str9[sizeof("N")];
    char general_category_stringpool_str10[sizeof("Pc")];
    char general_category_stringpool_str11[sizeof("Cn")];
    char general_category_stringpool_str12[sizeof("Lt")];
    char general_category_stringpool_str13[sizeof("P")];
    char general_category_stringpool_str15[sizeof("Control")];
    char general_category_stringpool_str16[sizeof("Lo")];
    char general_category_stringpool_str17[sizeof("Co")];
    char general_category_stringpool_str18[sizeof("Lu")];
    char general_category_stringpool_str20[sizeof("No")];
    char general_category_stringpool_str21[sizeof("Cf")];
    char general_category_stringpool_str22[sizeof("Po")];
    char general_category_stringpool_str23[sizeof("OtherSymbol")];
    char general_category_stringpool_str24[sizeof("CurrencySymbol")];
    char general_category_stringpool_str25[sizeof("Currency Symbol")];
    char general_category_stringpool_str26[sizeof("Pf")];
    char general_category_stringpool_str27[sizeof("Format")];
    char general_category_stringpool_str28[sizeof("Close Punctuation")];
    char general_category_stringpool_str29[sizeof("ClosePunctuation")];
    char general_category_stringpool_str36[sizeof("OpenPunctuation")];
    char general_category_stringpool_str37[sizeof("ConnectorPunctuation")];
    char general_category_stringpool_str38[sizeof("Connector Punctuation")];
    char general_category_stringpool_str39[sizeof("Other Punctuation")];
    char general_category_stringpool_str40[sizeof("OtherPunctuation")];
    char general_category_stringpool_str41[sizeof("Open Punctuation")];
    char general_category_stringpool_str42[sizeof("Other")];
    char general_category_stringpool_str43[sizeof("Number")];
    char general_category_stringpool_str45[sizeof("Punctuation")];
    char general_category_stringpool_str46[sizeof("Sc")];
    char general_category_stringpool_str47[sizeof("Zl")];
    char general_category_stringpool_str48[sizeof("Symbol")];
    char general_category_stringpool_str49[sizeof("Other Letter")];
    char general_category_stringpool_str50[sizeof("Pe")];
    char general_category_stringpool_str52[sizeof("Letter")];
    char general_category_stringpool_str53[sizeof("Other Number")];
    char general_category_stringpool_str54[sizeof("Cased Letter")];
    char general_category_stringpool_str55[sizeof("Mc")];
    char general_category_stringpool_str56[sizeof("OtherNumber")];
    char general_category_stringpool_str58[sizeof("So")];
    char general_category_stringpool_str59[sizeof("Zp")];
    char general_category_stringpool_str60[sizeof("Letter Number")];
    char general_category_stringpool_str61[sizeof("Mn")];
    char general_category_stringpool_str62[sizeof("LetterNumber")];
    char general_category_stringpool_str63[sizeof("LowercaseLetter")];
    char general_category_stringpool_str64[sizeof("Lowercase Letter")];
    char general_category_stringpool_str66[sizeof("Other Symbol")];
    char general_category_stringpool_str69[sizeof("OtherLetter")];
    char general_category_stringpool_str70[sizeof("UppercaseLetter")];
    char general_category_stringpool_str71[sizeof("Uppercase Letter")];
    char general_category_stringpool_str74[sizeof("CasedLetter")];
    char general_category_stringpool_str75[sizeof("NonspacingMark")];
    char general_category_stringpool_str76[sizeof("Nonspacing Mark")];
    char general_category_stringpool_str78[sizeof("Math Symbol")];
    char general_category_stringpool_str81[sizeof("ParagraphSeparator")];
    char general_category_stringpool_str82[sizeof("Paragraph Separator")];
    char general_category_stringpool_str83[sizeof("PrivateUse")];
    char general_category_stringpool_str84[sizeof("Private Use")];
    char general_category_stringpool_str85[sizeof("S")];
    char general_category_stringpool_str87[sizeof("Cs")];
    char general_category_stringpool_str88[sizeof("InitialPunctuation")];
    char general_category_stringpool_str89[sizeof("Initial Punctuation")];
    char general_category_stringpool_str91[sizeof("Z")];
    char general_category_stringpool_str92[sizeof("Ps")];
    char general_category_stringpool_str93[sizeof("ModifierSymbol")];
    char general_category_stringpool_str94[sizeof("Modifier Symbol")];
    char general_category_stringpool_str95[sizeof("Me")];
    char general_category_stringpool_str96[sizeof("Surrogate")];
    char general_category_stringpool_str98[sizeof("Final Punctuation")];
    char general_category_stringpool_str99[sizeof("FinalPunctuation")];
    char general_category_stringpool_str102[sizeof("Separator")];
    char general_category_stringpool_str103[sizeof("M")];
    char general_category_stringpool_str104[sizeof("Lm")];
    char general_category_stringpool_str105[sizeof("DashPunctuation")];
    char general_category_stringpool_str106[sizeof("LineSeparator")];
    char general_category_stringpool_str108[sizeof("SpaceSeparator")];
    char general_category_stringpool_str110[sizeof("Dash Punctuation")];
    char general_category_stringpool_str113[sizeof("Unassigned")];
    char general_category_stringpool_str118[sizeof("ModifierLetter")];
    char general_category_stringpool_str119[sizeof("Modifier Letter")];
    char general_category_stringpool_str120[sizeof("Mark")];
    char general_category_stringpool_str122[sizeof("Line Separator")];
    char general_category_stringpool_str123[sizeof("TitlecaseLetter")];
    char general_category_stringpool_str124[sizeof("Titlecase Letter")];
    char general_category_stringpool_str125[sizeof("DecimalNumber")];
    char general_category_stringpool_str126[sizeof("Decimal Number")];
    char general_category_stringpool_str128[sizeof("MathSymbol")];
    char general_category_stringpool_str130[sizeof("Space Separator")];
    char general_category_stringpool_str131[sizeof("Zs")];
    char general_category_stringpool_str132[sizeof("Pi")];
    char general_category_stringpool_str133[sizeof("SpacingMark")];
    char general_category_stringpool_str134[sizeof("Spacing Mark")];
    char general_category_stringpool_str138[sizeof("Nd")];
    char general_category_stringpool_str140[sizeof("Pd")];
    char general_category_stringpool_str142[sizeof("Sk")];
    char general_category_stringpool_str146[sizeof("Sm")];
    char general_category_stringpool_str149[sizeof("EnclosingMark")];
    char general_category_stringpool_str150[sizeof("Enclosing Mark")];
  };
static const struct general_category_stringpool_t general_category_stringpool_contents =
  {
    "L",
    "Ll",
    "C",
    "LC",
    "Cc",
    "Nl",
    "N",
    "Pc",
    "Cn",
    "Lt",
    "P",
    "Control",
    "Lo",
    "Co",
    "Lu",
    "No",
    "Cf",
    "Po",
    "OtherSymbol",
    "CurrencySymbol",
    "Currency Symbol",
    "Pf",
    "Format",
    "Close Punctuation",
    "ClosePunctuation",
    "OpenPunctuation",
    "ConnectorPunctuation",
    "Connector Punctuation",
    "Other Punctuation",
    "OtherPunctuation",
    "Open Punctuation",
    "Other",
    "Number",
    "Punctuation",
    "Sc",
    "Zl",
    "Symbol",
    "Other Letter",
    "Pe",
    "Letter",
    "Other Number",
    "Cased Letter",
    "Mc",
    "OtherNumber",
    "So",
    "Zp",
    "Letter Number",
    "Mn",
    "LetterNumber",
    "LowercaseLetter",
    "Lowercase Letter",
    "Other Symbol",
    "OtherLetter",
    "UppercaseLetter",
    "Uppercase Letter",
    "CasedLetter",
    "NonspacingMark",
    "Nonspacing Mark",
    "Math Symbol",
    "ParagraphSeparator",
    "Paragraph Separator",
    "PrivateUse",
    "Private Use",
    "S",
    "Cs",
    "InitialPunctuation",
    "Initial Punctuation",
    "Z",
    "Ps",
    "ModifierSymbol",
    "Modifier Symbol",
    "Me",
    "Surrogate",
    "Final Punctuation",
    "FinalPunctuation",
    "Separator",
    "M",
    "Lm",
    "DashPunctuation",
    "LineSeparator",
    "SpaceSeparator",
    "Dash Punctuation",
    "Unassigned",
    "ModifierLetter",
    "Modifier Letter",
    "Mark",
    "Line Separator",
    "TitlecaseLetter",
    "Titlecase Letter",
    "DecimalNumber",
    "Decimal Number",
    "MathSymbol",
    "Space Separator",
    "Zs",
    "Pi",
    "SpacingMark",
    "Spacing Mark",
    "Nd",
    "Pd",
    "Sk",
    "Sm",
    "EnclosingMark",
    "Enclosing Mark"
  };
#define general_category_stringpool ((const char *) &general_category_stringpool_contents)

static const struct named_category general_category_names[] =
  {
    {-1},
#line 14 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str1, UC_CATEGORY_INDEX_L},
#line 17 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str2, UC_CATEGORY_INDEX_Ll},
#line 46 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str3, UC_CATEGORY_INDEX_C},
#line 15 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str4, UC_CATEGORY_INDEX_LC},
#line 47 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str5, UC_CATEGORY_INDEX_Cc},
#line 27 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str6, UC_CATEGORY_INDEX_Nl},
    {-1}, {-1},
#line 25 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str9, UC_CATEGORY_INDEX_N},
#line 30 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str10, UC_CATEGORY_INDEX_Pc},
#line 51 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str11, UC_CATEGORY_INDEX_Cn},
#line 18 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str12, UC_CATEGORY_INDEX_Lt},
#line 29 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str13, UC_CATEGORY_INDEX_P},
    {-1},
#line 111 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str15, UC_CATEGORY_INDEX_Cc},
#line 20 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str16, UC_CATEGORY_INDEX_Lo},
#line 50 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str17, UC_CATEGORY_INDEX_Co},
#line 16 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str18, UC_CATEGORY_INDEX_Lu},
    {-1},
#line 28 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str20, UC_CATEGORY_INDEX_No},
#line 48 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str21, UC_CATEGORY_INDEX_Cf},
#line 36 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str22, UC_CATEGORY_INDEX_Po},
#line 102 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str23, UC_CATEGORY_INDEX_So},
#line 98 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str24, UC_CATEGORY_INDEX_Sc},
#line 97 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str25, UC_CATEGORY_INDEX_Sc},
#line 35 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str26, UC_CATEGORY_INDEX_Pf},
#line 112 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str27, UC_CATEGORY_INDEX_Cf},
#line 86 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str28, UC_CATEGORY_INDEX_Pe},
#line 87 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str29, UC_CATEGORY_INDEX_Pe},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 85 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str36, UC_CATEGORY_INDEX_Ps},
#line 81 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str37, UC_CATEGORY_INDEX_Pc},
#line 80 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str38, UC_CATEGORY_INDEX_Pc},
#line 92 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str39, UC_CATEGORY_INDEX_Po},
#line 93 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str40, UC_CATEGORY_INDEX_Po},
#line 84 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str41, UC_CATEGORY_INDEX_Ps},
#line 110 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str42, UC_CATEGORY_INDEX_C},
#line 72 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str43, UC_CATEGORY_INDEX_N},
    {-1},
#line 79 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str45, UC_CATEGORY_INDEX_P},
#line 39 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str46, UC_CATEGORY_INDEX_Sc},
#line 44 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str47, UC_CATEGORY_INDEX_Zl},
#line 94 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str48, UC_CATEGORY_INDEX_S},
#line 63 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str49, UC_CATEGORY_INDEX_Lo},
#line 33 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str50, UC_CATEGORY_INDEX_Pe},
    {-1},
#line 52 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str52, UC_CATEGORY_INDEX_L},
#line 77 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str53, UC_CATEGORY_INDEX_No},
#line 53 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str54, UC_CATEGORY_INDEX_LC},
#line 23 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str55, UC_CATEGORY_INDEX_Mc},
#line 78 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str56, UC_CATEGORY_INDEX_No},
    {-1},
#line 41 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str58, UC_CATEGORY_INDEX_So},
#line 45 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str59, UC_CATEGORY_INDEX_Zp},
#line 75 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str60, UC_CATEGORY_INDEX_Nl},
#line 22 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str61, UC_CATEGORY_INDEX_Mn},
#line 76 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str62, UC_CATEGORY_INDEX_Nl},
#line 58 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str63, UC_CATEGORY_INDEX_Ll},
#line 57 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str64, UC_CATEGORY_INDEX_Ll},
    {-1},
#line 101 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str66, UC_CATEGORY_INDEX_So},
    {-1}, {-1},
#line 64 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str69, UC_CATEGORY_INDEX_Lo},
#line 56 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str70, UC_CATEGORY_INDEX_Lu},
#line 55 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str71, UC_CATEGORY_INDEX_Lu},
    {-1}, {-1},
#line 54 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str74, UC_CATEGORY_INDEX_LC},
#line 67 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str75, UC_CATEGORY_INDEX_Mn},
#line 66 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str76, UC_CATEGORY_INDEX_Mn},
    {-1},
#line 95 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str78, UC_CATEGORY_INDEX_Sm},
    {-1}, {-1},
#line 109 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str81, UC_CATEGORY_INDEX_Zp},
#line 108 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str82, UC_CATEGORY_INDEX_Zp},
#line 115 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str83, UC_CATEGORY_INDEX_Co},
#line 114 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str84, UC_CATEGORY_INDEX_Co},
#line 37 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str85, UC_CATEGORY_INDEX_S},
    {-1},
#line 49 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str87, UC_CATEGORY_INDEX_Cs},
#line 89 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str88, UC_CATEGORY_INDEX_Pi},
#line 88 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str89, UC_CATEGORY_INDEX_Pi},
    {-1},
#line 42 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str91, UC_CATEGORY_INDEX_Z},
#line 32 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str92, UC_CATEGORY_INDEX_Ps},
#line 100 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str93, UC_CATEGORY_INDEX_Sk},
#line 99 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str94, UC_CATEGORY_INDEX_Sk},
#line 24 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str95, UC_CATEGORY_INDEX_Me},
#line 113 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str96, UC_CATEGORY_INDEX_Cs},
    {-1},
#line 90 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str98, UC_CATEGORY_INDEX_Pf},
#line 91 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str99, UC_CATEGORY_INDEX_Pf},
    {-1}, {-1},
#line 103 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str102, UC_CATEGORY_INDEX_Z},
#line 21 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str103, UC_CATEGORY_INDEX_M},
#line 19 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str104, UC_CATEGORY_INDEX_Lm},
#line 83 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str105, UC_CATEGORY_INDEX_Pd},
#line 107 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str106, UC_CATEGORY_INDEX_Zl},
    {-1},
#line 105 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str108, UC_CATEGORY_INDEX_Zs},
    {-1},
#line 82 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str110, UC_CATEGORY_INDEX_Pd},
    {-1}, {-1},
#line 116 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str113, UC_CATEGORY_INDEX_Cn},
    {-1}, {-1}, {-1}, {-1},
#line 62 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str118, UC_CATEGORY_INDEX_Lm},
#line 61 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str119, UC_CATEGORY_INDEX_Lm},
#line 65 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str120, UC_CATEGORY_INDEX_M},
    {-1},
#line 106 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str122, UC_CATEGORY_INDEX_Zl},
#line 60 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str123, UC_CATEGORY_INDEX_Lt},
#line 59 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str124, UC_CATEGORY_INDEX_Lt},
#line 74 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str125, UC_CATEGORY_INDEX_Nd},
#line 73 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str126, UC_CATEGORY_INDEX_Nd},
    {-1},
#line 96 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str128, UC_CATEGORY_INDEX_Sm},
    {-1},
#line 104 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str130, UC_CATEGORY_INDEX_Zs},
#line 43 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str131, UC_CATEGORY_INDEX_Zs},
#line 34 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str132, UC_CATEGORY_INDEX_Pi},
#line 69 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str133, UC_CATEGORY_INDEX_Mc},
#line 68 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str134, UC_CATEGORY_INDEX_Mc},
    {-1}, {-1}, {-1},
#line 26 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str138, UC_CATEGORY_INDEX_Nd},
    {-1},
#line 31 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str140, UC_CATEGORY_INDEX_Pd},
    {-1},
#line 40 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str142, UC_CATEGORY_INDEX_Sk},
    {-1}, {-1}, {-1},
#line 38 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str146, UC_CATEGORY_INDEX_Sm},
    {-1}, {-1},
#line 71 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str149, UC_CATEGORY_INDEX_Me},
#line 70 "../../../lib/unistring/unictype/categ_byname.gperf"
    {(int)(size_t)&((struct general_category_stringpool_t *)0)->general_category_stringpool_str150, UC_CATEGORY_INDEX_Me}
  };

const struct named_category *
uc_general_category_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = general_category_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register int o = general_category_names[key].name;
          if (o >= 0)
            {
              register const char *s = o + general_category_stringpool;

              if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_strcmp (str, s))
                return &general_category_names[key];
            }
        }
    }
  return 0;
}
