/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m 1 ./uninorm/composition-table.gperf  */
/* Computed positions: -k'2-3,6' */


#define TOTAL_KEYWORDS 940
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 6
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 1565
/* maximum key range = 1565, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
gl_uninorm_compose_hash (register const char *str, register size_t len)
{
  static const unsigned short asso_values[] =
    {
         7,    1,    0,    3,   58,  132,  531,   62,    4,   33,
       117,  268,  476,  135,  509,  481,  103,  265,  249,  495,
        61,  124,  336,  409,  703,  241,  435,  462,  738, 1566,
        48,   13,  901,  766,  322,  635,  192,  621,  846,   13,
       337,   65,  161,  130,  103,   28,  255,  640,  133,  342,
       172,   52, 1566,  818,  201,   34,  695,   50,   31, 1566,
        16,   35,  438,  139,   63,   89,  272,    2,  588,  167,
        12,  375,  289,   61,   44,  548,  431,  452,  395,  180,
       794,  853,  362,  561,  456,  202,  677,  360,  195,  300,
       572,  715, 1566,  291, 1566,  276,  814,   26,  634,  579,
       270,   98,  423,  415,  323,   42,  862,  657,  559,  324,
       262,  124,  863, 1566,  251,  538,  236,  133,   47,  406,
       691,  219,  590,    5,   56,   83, 1566,    1, 1566, 1566,
        65,   39, 1566, 1566,   37,   35, 1566, 1566, 1566, 1566,
      1566, 1566, 1566, 1566,    4,  527,   18, 1566,    0,  849,
      1566,  781, 1566,  572,   59,   73, 1566,  204, 1566,  417,
       618,  511,   20, 1566, 1566,  708,  167, 1566,  855,  685,
      1566,  333,   73,  136,   71,  625,  611,  523,   12,  311,
        11,  649,   10,  505,   62,  482, 1566,  113, 1566,  126,
         7,  476, 1566,  614,  853,    1,   41,  560,  553,  261,
       104,  719,  668,  854,   19,   10,    9,  728,   31, 1566,
       442, 1566,  749,  313,   12,    0,  659,    3, 1566,  329,
        37, 1566, 1566, 1566,   44, 1566,  847, 1566,   33,    4,
        23,    1,   33,   24,  757,    4, 1566, 1566, 1566,    3,
        93,   73,   72, 1566,  824,   20,   17, 1566,    0, 1566,
      1566, 1566,   77,   46,  460, 1566, 1566
    };
  return asso_values[(unsigned char)str[5]+1] + asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[1]];
}

const struct composition_rule *
gl_uninorm_compose_lookup (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       0,  6,  6,  0,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  6,  6,
       6,  6,  6,  6,  6,  0,  6,  6,  6,  6,  6,  6,  6,  6,
       0,  6,  6,  6,  6,  6,  6,  6,  6,  0,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,
       6,  6,  6,  0,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  0,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  0,  6,  6,  0,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  0,  6,  6,  0,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  0,  6,  6,  6,  6,  6,  6,  0,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       0,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  6,  6,
       0,  6,  0,  6,  0,  6,  6,  6,  6,  0,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
       6,  6,  6,  6,  6,  6,  6,  0,  6,  6,  6,  6,  0,  6,
       6,  6,  0,  6,  0,  6,  6,  6,  6,  0,  6,  6,  0,  6,
       6,  6,  0,  6,  0,  0,  0,  6,  0,  6,  0,  6,  6,  6,
       6,  0,  6,  6,  6,  6,  6,  0,  6,  0,  6,  0,  0,  6,
       6,  6,  6,  0,  6,  0,  0,  6,  6,  0,  6,  6,  6,  0,
       6,  0,  0,  0,  6,  0,  6,  6,  0,  6,  6,  0,  0,  0,
       0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  6,  0,
       0,  0,  0,  0,  6,  6,  0,  0,  0,  0,  6,  6,  6,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,
       0,  0,  0,  0,  0,  0,  6,  6,  0,  6,  6,  6,  0,  6,
       0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,
       6,  0,  0,  0,  6,  0,  0,  6,  6,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  6,  0,  0,  0,  6,  6,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,
       0,  6,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,  6,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,
       0,  0,  0,  6,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6
    };
  static const struct composition_rule wordlist[] =
    {
      {""},
#line 581 "./uninorm/composition-table.gperf"
      {"\000\001\002\000\003\001", 0x1eae},
#line 583 "./uninorm/composition-table.gperf"
      {"\000\001\002\000\003\000", 0x1eb0},
      {""},
#line 582 "./uninorm/composition-table.gperf"
      {"\000\001\003\000\003\001", 0x1eaf},
#line 584 "./uninorm/composition-table.gperf"
      {"\000\001\003\000\003\000", 0x1eb1},
#line 566 "./uninorm/composition-table.gperf"
      {"\000\001\177\000\003\007", 0x1e9b},
#line 247 "./uninorm/composition-table.gperf"
      {"\000\000\370\000\003\001", 0x01ff},
#line 421 "./uninorm/composition-table.gperf"
      {"\000\000\347\000\003\001", 0x1e09},
#line 99 "./uninorm/composition-table.gperf"
      {"\000\000C\000\003\001", 0x0106},
#line 459 "./uninorm/composition-table.gperf"
      {"\000\000\357\000\003\001", 0x1e2f},
#line 243 "./uninorm/composition-table.gperf"
      {"\000\000\345\000\003\001", 0x01fb},
#line 101 "./uninorm/composition-table.gperf"
      {"\000\000C\000\003\002", 0x0108},
#line 103 "./uninorm/composition-table.gperf"
      {"\000\000C\000\003\007", 0x010a},
#line 662 "./uninorm/composition-table.gperf"
      {"\000\037\001\000\003\001", 0x1f05},
#line 660 "./uninorm/composition-table.gperf"
      {"\000\037\001\000\003\000", 0x1f03},
#line 664 "./uninorm/composition-table.gperf"
      {"\000\037\001\000\003B", 0x1f07},
#line 669 "./uninorm/composition-table.gperf"
      {"\000\037\010\000\003\001", 0x1f0c},
#line 667 "./uninorm/composition-table.gperf"
      {"\000\037\010\000\003\000", 0x1f0a},
#line 671 "./uninorm/composition-table.gperf"
      {"\000\037\010\000\003B", 0x1f0e},
#line 661 "./uninorm/composition-table.gperf"
      {"\000\037\000\000\003\001", 0x1f04},
#line 659 "./uninorm/composition-table.gperf"
      {"\000\037\000\000\003\000", 0x1f02},
#line 663 "./uninorm/composition-table.gperf"
      {"\000\037\000\000\003B", 0x1f06},
#line 442 "./uninorm/composition-table.gperf"
      {"\000\000F\000\003\007", 0x1e1e},
#line 860 "./uninorm/composition-table.gperf"
      {"\000\003\316\000\003E", 0x1ff4},
#line 766 "./uninorm/composition-table.gperf"
      {"\000\037\002\000\003E", 0x1f82},
#line 765 "./uninorm/composition-table.gperf"
      {"\000\037\001\000\003E", 0x1f81},
#line 489 "./uninorm/composition-table.gperf"
      {"\000\000\365\000\003\001", 0x1e4d},
#line 767 "./uninorm/composition-table.gperf"
      {"\000\037\003\000\003E", 0x1f83},
#line 772 "./uninorm/composition-table.gperf"
      {"\000\037\010\000\003E", 0x1f88},
#line 245 "./uninorm/composition-table.gperf"
      {"\000\000\346\000\003\001", 0x01fd},
#line 515 "./uninorm/composition-table.gperf"
      {"\000\001a\000\003\007", 0x1e67},
#line 764 "./uninorm/composition-table.gperf"
      {"\000\037\000\000\003E", 0x1f80},
#line 67 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\001", 0x00e1},
#line 66 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\000", 0x00e0},
#line 818 "./uninorm/composition-table.gperf"
      {"\000\037\266\000\003E", 0x1fb7},
#line 68 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\002", 0x00e2},
#line 279 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\007", 0x0227},
#line 787 "./uninorm/composition-table.gperf"
      {"\000\037'\000\003E", 0x1f97},
#line 746 "./uninorm/composition-table.gperf"
      {"\000\037a\000\003\001", 0x1f65},
#line 744 "./uninorm/composition-table.gperf"
      {"\000\037a\000\003\000", 0x1f63},
#line 748 "./uninorm/composition-table.gperf"
      {"\000\037a\000\003B", 0x1f67},
#line 862 "./uninorm/composition-table.gperf"
      {"\000\037\366\000\003E", 0x1ff7},
#line 533 "./uninorm/composition-table.gperf"
      {"\000\001i\000\003\001", 0x1e79},
#line 215 "./uninorm/composition-table.gperf"
      {"\000\000\334\000\003\001", 0x01d7},
#line 219 "./uninorm/composition-table.gperf"
      {"\000\000\334\000\003\000", 0x01db},
#line 670 "./uninorm/composition-table.gperf"
      {"\000\037\011\000\003\001", 0x1f0d},
#line 668 "./uninorm/composition-table.gperf"
      {"\000\037\011\000\003\000", 0x1f0b},
#line 672 "./uninorm/composition-table.gperf"
      {"\000\037\011\000\003B", 0x1f0f},
#line 78 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\001", 0x00ed},
#line 77 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\000", 0x00ec},
#line 797 "./uninorm/composition-table.gperf"
      {"\000\037a\000\003E", 0x1fa1},
#line 79 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\002", 0x00ee},
#line 793 "./uninorm/composition-table.gperf"
      {"\000\037-\000\003E", 0x1f9d},
#line 138 "./uninorm/composition-table.gperf"
      {"\000\000J\000\003\002", 0x0134},
#line 754 "./uninorm/composition-table.gperf"
      {"\000\037i\000\003\001", 0x1f6d},
#line 752 "./uninorm/composition-table.gperf"
      {"\000\037i\000\003\000", 0x1f6b},
#line 756 "./uninorm/composition-table.gperf"
      {"\000\037i\000\003B", 0x1f6f},
#line 773 "./uninorm/composition-table.gperf"
      {"\000\037\011\000\003E", 0x1f89},
#line 587 "./uninorm/composition-table.gperf"
      {"\000\001\002\000\003\003", 0x1eb4},
#line 491 "./uninorm/composition-table.gperf"
      {"\000\000\365\000\003\010", 0x1e4f},
#line 624 "./uninorm/composition-table.gperf"
      {"\000\036\315\000\003\002", 0x1ed9},
#line 588 "./uninorm/composition-table.gperf"
      {"\000\001\003\000\003\003", 0x1eb5},
#line 714 "./uninorm/composition-table.gperf"
      {"\000\0379\000\003\001", 0x1f3d},
#line 712 "./uninorm/composition-table.gperf"
      {"\000\0379\000\003\000", 0x1f3b},
#line 716 "./uninorm/composition-table.gperf"
      {"\000\0379\000\003B", 0x1f3f},
#line 70 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\010", 0x00e4},
#line 805 "./uninorm/composition-table.gperf"
      {"\000\037i\000\003E", 0x1fa9},
#line 52 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\001", 0x00cd},
#line 51 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\000", 0x00cc},
#line 623 "./uninorm/composition-table.gperf"
      {"\000\036\314\000\003\002", 0x1ed8},
#line 53 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\002", 0x00ce},
#line 137 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\007", 0x0130},
#line 884 "./uninorm/composition-table.gperf"
      {"\000\000<\000\0038", 0x226e},
#line 728 "./uninorm/composition-table.gperf"
      {"\000\037I\000\003\001", 0x1f4d},
#line 726 "./uninorm/composition-table.gperf"
      {"\000\037I\000\003\000", 0x1f4b},
#line 721 "./uninorm/composition-table.gperf"
      {"\000\037@\000\003\001", 0x1f44},
#line 719 "./uninorm/composition-table.gperf"
      {"\000\037@\000\003\000", 0x1f42},
#line 698 "./uninorm/composition-table.gperf"
      {"\000\037)\000\003\001", 0x1f2d},
#line 696 "./uninorm/composition-table.gperf"
      {"\000\037)\000\003\000", 0x1f2b},
#line 700 "./uninorm/composition-table.gperf"
      {"\000\037)\000\003B", 0x1f2f},
#line 858 "./uninorm/composition-table.gperf"
      {"\000\037|\000\003E", 0x1ff2},
#line 80 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\010", 0x00ef},
#line 768 "./uninorm/composition-table.gperf"
      {"\000\037\004\000\003E", 0x1f84},
#line 216 "./uninorm/composition-table.gperf"
      {"\000\000\374\000\003\001", 0x01d8},
#line 220 "./uninorm/composition-table.gperf"
      {"\000\000\374\000\003\000", 0x01dc},
#line 826 "./uninorm/composition-table.gperf"
      {"\000\003\256\000\003E", 0x1fc4},
#line 771 "./uninorm/composition-table.gperf"
      {"\000\037\007\000\003E", 0x1f87},
#line 816 "./uninorm/composition-table.gperf"
      {"\000\003\254\000\003E", 0x1fb4},
#line 328 "./uninorm/composition-table.gperf"
      {"\000\004:\000\003\001", 0x045c},
#line 789 "./uninorm/composition-table.gperf"
      {"\000\037)\000\003E", 0x1f99},
#line 69 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\003", 0x00e3},
#line 881 "./uninorm/composition-table.gperf"
      {"\000\000=\000\0038", 0x2260},
#line 360 "./uninorm/composition-table.gperf"
      {"\000\004C\000\003\010", 0x04f1},
#line 342 "./uninorm/composition-table.gperf"
      {"\000\004\331\000\003\010", 0x04db},
#line 96 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\006", 0x0103},
#line 41 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\001", 0x00c1},
#line 40 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\000", 0x00c0},
#line 98 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003(", 0x0105},
#line 42 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\002", 0x00c2},
#line 278 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\007", 0x0226},
#line 54 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\010", 0x00cf},
#line 722 "./uninorm/composition-table.gperf"
      {"\000\037A\000\003\001", 0x1f45},
#line 720 "./uninorm/composition-table.gperf"
      {"\000\037A\000\003\000", 0x1f43},
#line 363 "./uninorm/composition-table.gperf"
      {"\000\004'\000\003\010", 0x04f4},
#line 74 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\001", 0x00e9},
#line 73 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\000", 0x00e8},
#line 130 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\003", 0x0129},
#line 75 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\002", 0x00ea},
#line 114 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\007", 0x0117},
#line 326 "./uninorm/composition-table.gperf"
      {"\000\0043\000\003\001", 0x0453},
#line 134 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\006", 0x012d},
#line 537 "./uninorm/composition-table.gperf"
      {"\000\000v\000\003\003", 0x1e7d},
#line 605 "./uninorm/composition-table.gperf"
      {"\000\036\270\000\003\002", 0x1ec6},
#line 136 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003(", 0x012f},
#line 354 "./uninorm/composition-table.gperf"
      {"\000\004\351\000\003\010", 0x04eb},
#line 677 "./uninorm/composition-table.gperf"
      {"\000\037\020\000\003\001", 0x1f14},
#line 675 "./uninorm/composition-table.gperf"
      {"\000\037\020\000\003\000", 0x1f12},
#line 585 "./uninorm/composition-table.gperf"
      {"\000\001\002\000\003\011", 0x1eb2},
#line 355 "./uninorm/composition-table.gperf"
      {"\000\004-\000\003\010", 0x04ec},
#line 377 "./uninorm/composition-table.gperf"
      {"\000\0113\000\011<", 0x0934},
#line 586 "./uninorm/composition-table.gperf"
      {"\000\001\003\000\003\011", 0x1eb3},
#line 330 "./uninorm/composition-table.gperf"
      {"\000\004C\000\003\006", 0x045e},
#line 801 "./uninorm/composition-table.gperf"
      {"\000\037e\000\003E", 0x1fa5},
#line 353 "./uninorm/composition-table.gperf"
      {"\000\004\350\000\003\010", 0x04ea},
#line 346 "./uninorm/composition-table.gperf"
      {"\000\0047\000\003\010", 0x04df},
#line 129 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\003", 0x0128},
#line 441 "./uninorm/composition-table.gperf"
      {"\000\002)\000\003\006", 0x1e1d},
#line 792 "./uninorm/composition-table.gperf"
      {"\000\037,\000\003E", 0x1f9c},
#line 44 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\010", 0x00c4},
#line 133 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\006", 0x012c},
#line 83 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\001", 0x00f3},
#line 82 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\000", 0x00f2},
#line 135 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003(", 0x012e},
#line 84 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\002", 0x00f4},
#line 287 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\007", 0x022f},
#line 249 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\017", 0x0201},
#line 234 "./uninorm/composition-table.gperf"
      {"\000\001\353\000\003\004", 0x01ed},
#line 76 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\010", 0x00eb},
#line 351 "./uninorm/composition-table.gperf"
      {"\000\004\036\000\003\010", 0x04e6},
#line 88 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\001", 0x00fa},
#line 87 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\000", 0x00f9},
#line 774 "./uninorm/composition-table.gperf"
      {"\000\037\012\000\003E", 0x1f8a},
#line 89 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\002", 0x00fb},
#line 105 "./uninorm/composition-table.gperf"
      {"\000\000C\000\003\014", 0x010c},
#line 224 "./uninorm/composition-table.gperf"
      {"\000\002'\000\003\004", 0x01e1},
#line 705 "./uninorm/composition-table.gperf"
      {"\000\0370\000\003\001", 0x1f34},
#line 703 "./uninorm/composition-table.gperf"
      {"\000\0370\000\003\000", 0x1f32},
#line 707 "./uninorm/composition-table.gperf"
      {"\000\0370\000\003B", 0x1f36},
#line 811 "./uninorm/composition-table.gperf"
      {"\000\037o\000\003E", 0x1faf},
#line 570 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\011", 0x1ea3},
#line 282 "./uninorm/composition-table.gperf"
      {"\000\000\326\000\003\004", 0x022a},
#line 257 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\017", 0x0209},
#line 236 "./uninorm/composition-table.gperf"
      {"\000\002\222\000\003\014", 0x01ef},
#line 43 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\003", 0x00c3},
#line 791 "./uninorm/composition-table.gperf"
      {"\000\037+\000\003E", 0x1f9b},
#line 283 "./uninorm/composition-table.gperf"
      {"\000\000\366\000\003\004", 0x022b},
#line 769 "./uninorm/composition-table.gperf"
      {"\000\037\005\000\003E", 0x1f85},
#line 95 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\006", 0x0102},
#line 285 "./uninorm/composition-table.gperf"
      {"\000\000\365\000\003\004", 0x022d},
#line 777 "./uninorm/composition-table.gperf"
      {"\000\037\015\000\003E", 0x1f8d},
#line 97 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003(", 0x0104},
#line 226 "./uninorm/composition-table.gperf"
      {"\000\000\346\000\003\004", 0x01e3},
#line 596 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\003", 0x1ebd},
#line 86 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\010", 0x00f6},
#line 94 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\004", 0x0101},
#line 608 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\011", 0x1ec9},
#line 112 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\006", 0x0115},
#line 206 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\014", 0x01ce},
#line 397 "./uninorm/composition-table.gperf"
      {"\000\015\331\000\015\317", 0x0ddc},
#line 116 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003(", 0x0119},
#line 256 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\017", 0x0208},
#line 222 "./uninorm/composition-table.gperf"
      {"\000\000\344\000\003\004", 0x01df},
#line 90 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\010", 0x00fc},
#line 48 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\001", 0x00c9},
#line 47 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\000", 0x00c8},
#line 213 "./uninorm/composition-table.gperf"
      {"\000\000\334\000\003\004", 0x01d5},
#line 49 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\002", 0x00ca},
#line 113 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\007", 0x0116},
#line 217 "./uninorm/composition-table.gperf"
      {"\000\000\334\000\003\014", 0x01d9},
#line 221 "./uninorm/composition-table.gperf"
      {"\000\000\304\000\003\004", 0x01de},
#line 132 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\004", 0x012b},
#line 399 "./uninorm/composition-table.gperf"
      {"\000\015\331\000\015\337", 0x0dde},
#line 315 "./uninorm/composition-table.gperf"
      {"\000\004\025\000\003\000", 0x0400},
#line 208 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\014", 0x01d0},
#line 607 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\011", 0x1ec8},
#line 790 "./uninorm/composition-table.gperf"
      {"\000\037*\000\003E", 0x1f9a},
#line 57 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\001", 0x00d3},
#line 56 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\000", 0x00d2},
#line 85 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\003", 0x00f5},
#line 58 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\002", 0x00d4},
#line 286 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\007", 0x022e},
#line 358 "./uninorm/composition-table.gperf"
      {"\000\004C\000\003\004", 0x04ef},
#line 157 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\006", 0x014f},
#line 337 "./uninorm/composition-table.gperf"
      {"\000\004\020\000\003\010", 0x04d2},
#line 963 "./uninorm/composition-table.gperf"
      {"\0000\357\0000\231", 0x30f7},
#line 232 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003(", 0x01eb},
#line 933 "./uninorm/composition-table.gperf"
      {"\0000{\0000\231", 0x307c},
#line 179 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\003", 0x0169},
#line 248 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\017", 0x0200},
#line 131 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\004", 0x012a},
#line 376 "./uninorm/composition-table.gperf"
      {"\000\0110\000\011<", 0x0931},
#line 183 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\006", 0x016d},
#line 207 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\014", 0x01cf},
#line 935 "./uninorm/composition-table.gperf"
      {"\0000F\0000\231", 0x3094},
#line 189 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003(", 0x0173},
#line 550 "./uninorm/composition-table.gperf"
      {"\000\000X\000\003\007", 0x1e8a},
#line 50 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\010", 0x00cb},
#line 253 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\017", 0x0205},
#line 62 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\001", 0x00da},
#line 61 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\000", 0x00d9},
#line 934 "./uninorm/composition-table.gperf"
      {"\0000{\0000\232", 0x307d},
#line 63 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\002", 0x00db},
#line 569 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\011", 0x1ea2},
#line 469 "./uninorm/composition-table.gperf"
      {"\000\0367\000\003\004", 0x1e39},
#line 316 "./uninorm/composition-table.gperf"
      {"\000\004\025\000\003\010", 0x0401},
#line 214 "./uninorm/composition-table.gperf"
      {"\000\000\374\000\003\004", 0x01d6},
#line 784 "./uninorm/composition-table.gperf"
      {"\000\037$\000\003E", 0x1f94},
#line 921 "./uninorm/composition-table.gperf"
      {"\0000a\0000\231", 0x3062},
#line 218 "./uninorm/composition-table.gperf"
      {"\000\000\374\000\003\014", 0x01da},
#line 60 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\010", 0x00d6},
#line 365 "./uninorm/composition-table.gperf"
      {"\000\004+\000\003\010", 0x04f8},
#line 594 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\011", 0x1ebb},
#line 335 "./uninorm/composition-table.gperf"
      {"\000\004\020\000\003\006", 0x04d0},
#line 338 "./uninorm/composition-table.gperf"
      {"\000\0040\000\003\010", 0x04d3},
#line 568 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003#", 0x1ea1},
#line 91 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\001", 0x00fd},
#line 650 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\000", 0x1ef3},
#line 93 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\004", 0x0100},
#line 193 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\002", 0x0177},
#line 555 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\007", 0x1e8f},
#line 205 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\014", 0x01cd},
#line 595 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\003", 0x1ebc},
#line 949 "./uninorm/composition-table.gperf"
      {"\0000\304\0000\231", 0x30c5},
#line 261 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\017", 0x020d},
#line 552 "./uninorm/composition-table.gperf"
      {"\000\000X\000\003\010", 0x1e8c},
#line 111 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\006", 0x0114},
#line 110 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\004", 0x0113},
#line 967 "./uninorm/composition-table.gperf"
      {"\0000\375\0000\231", 0x30fe},
#line 115 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003(", 0x0118},
#line 118 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\014", 0x011b},
#line 610 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003#", 0x1ecb},
#line 64 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\010", 0x00dc},
#line 269 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\017", 0x0215},
#line 339 "./uninorm/composition-table.gperf"
      {"\000\004\025\000\003\006", 0x04d6},
#line 59 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\003", 0x00d5},
#line 539 "./uninorm/composition-table.gperf"
      {"\000\000v\000\003#", 0x1e7f},
#line 519 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003\007", 0x1e6b},
#line 614 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\011", 0x1ecf},
#line 156 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\006", 0x014e},
#line 434 "./uninorm/composition-table.gperf"
      {"\000\001\022\000\003\001", 0x1e16},
#line 432 "./uninorm/composition-table.gperf"
      {"\000\001\022\000\003\000", 0x1e14},
#line 231 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003(", 0x01ea},
#line 336 "./uninorm/composition-table.gperf"
      {"\000\0040\000\003\006", 0x04d1},
#line 684 "./uninorm/composition-table.gperf"
      {"\000\037\031\000\003\001", 0x1f1d},
#line 682 "./uninorm/composition-table.gperf"
      {"\000\037\031\000\003\000", 0x1f1b},
#line 275 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003&", 0x021b},
#line 638 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\011", 0x1ee7},
#line 161 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003\001", 0x0155},
#line 92 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\010", 0x00ff},
#line 609 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003#", 0x1eca},
#line 824 "./uninorm/composition-table.gperf"
      {"\000\037t\000\003E", 0x1fc2},
#line 501 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003\007", 0x1e59},
#line 155 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\004", 0x014d},
#line 966 "./uninorm/composition-table.gperf"
      {"\0000\362\0000\231", 0x30fa},
#line 965 "./uninorm/composition-table.gperf"
      {"\0000\361\0000\231", 0x30f9},
#line 210 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\014", 0x01d2},
#line 178 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\003", 0x0168},
#line 420 "./uninorm/composition-table.gperf"
      {"\000\000\307\000\003\001", 0x1e08},
#line 149 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003\001", 0x0144},
#line 241 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003\000", 0x01f9},
#line 182 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\006", 0x016c},
#line 181 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\004", 0x016b},
#line 481 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003\007", 0x1e45},
#line 188 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003(", 0x0172},
#line 212 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\014", 0x01d4},
#line 563 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003\010", 0x1e97},
#line 252 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\017", 0x0204},
#line 678 "./uninorm/composition-table.gperf"
      {"\000\037\021\000\003\001", 0x1f15},
#line 676 "./uninorm/composition-table.gperf"
      {"\000\037\021\000\003\000", 0x1f13},
#line 794 "./uninorm/composition-table.gperf"
      {"\000\037.\000\003E", 0x1f9e},
#line 423 "./uninorm/composition-table.gperf"
      {"\000\000d\000\003\007", 0x1e0b},
#line 251 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\021", 0x0203},
#line 414 "./uninorm/composition-table.gperf"
      {"\000\000B\000\003\007", 0x1e02},
#line 656 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\003", 0x1ef9},
#line 964 "./uninorm/composition-table.gperf"
      {"\0000\360\0000\231", 0x30f8},
#line 393 "./uninorm/composition-table.gperf"
      {"\000\015F\000\015>", 0x0d4a},
#line 810 "./uninorm/composition-table.gperf"
      {"\000\037n\000\003E", 0x1fae},
#line 567 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003#", 0x1ea0},
#line 969 "./uninorm/composition-table.gperf"
      {"\001\020\233\001\020\272", 0x1109c},
#line 260 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\017", 0x020c},
#line 593 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\011", 0x1eba},
#line 344 "./uninorm/composition-table.gperf"
      {"\000\0046\000\003\010", 0x04dd},
#line 775 "./uninorm/composition-table.gperf"
      {"\000\037\013\000\003E", 0x1f8b},
#line 332 "./uninorm/composition-table.gperf"
      {"\000\004u\000\003\017", 0x0477},
#line 800 "./uninorm/composition-table.gperf"
      {"\000\037d\000\003E", 0x1fa4},
#line 951 "./uninorm/composition-table.gperf"
      {"\0000\310\0000\231", 0x30c9},
#line 592 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003#", 0x1eb9},
#line 259 "./uninorm/composition-table.gperf"
      {"\000\000i\000\003\021", 0x020b},
#line 127 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003\002", 0x0124},
#line 446 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003\007", 0x1e22},
#line 71 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003\012", 0x00e5},
#line 727 "./uninorm/composition-table.gperf"
      {"\000\037H\000\003\001", 0x1f4c},
#line 725 "./uninorm/composition-table.gperf"
      {"\000\037H\000\003\000", 0x1f4a},
#line 613 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\011", 0x1ece},
#line 945 "./uninorm/composition-table.gperf"
      {"\0000\273\0000\231", 0x30bc},
#line 109 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\004", 0x0112},
#line 65 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\001", 0x00dd},
#line 649 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\000", 0x1ef2},
#line 117 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\014", 0x011a},
#line 192 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\002", 0x0176},
#line 554 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\007", 0x1e8e},
#line 268 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\017", 0x0214},
#line 739 "./uninorm/composition-table.gperf"
      {"\000\037Y\000\003\001", 0x1f5d},
#line 738 "./uninorm/composition-table.gperf"
      {"\000\037Y\000\003\000", 0x1f5b},
#line 740 "./uninorm/composition-table.gperf"
      {"\000\037Y\000\003B", 0x1f5f},
#line 925 "./uninorm/composition-table.gperf"
      {"\0000o\0000\231", 0x3070},
#line 258 "./uninorm/composition-table.gperf"
      {"\000\000I\000\003\021", 0x020a},
#line 946 "./uninorm/composition-table.gperf"
      {"\0000\275\0000\231", 0x30be},
#line 154 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\004", 0x014c},
#line 488 "./uninorm/composition-table.gperf"
      {"\000\000\325\000\003\001", 0x1e4c},
#line 334 "./uninorm/composition-table.gperf"
      {"\000\0046\000\003\006", 0x04c2},
#line 209 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\014", 0x01d1},
#line 612 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003#", 0x1ecd},
#line 532 "./uninorm/composition-table.gperf"
      {"\000\001h\000\003\001", 0x1e78},
#line 929 "./uninorm/composition-table.gperf"
      {"\0000u\0000\231", 0x3076},
#line 637 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\011", 0x1ee6},
#line 81 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003\003", 0x00f1},
#line 938 "./uninorm/composition-table.gperf"
      {"\0000\255\0000\231", 0x30ae},
#line 450 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003\010", 0x1e26},
#line 926 "./uninorm/composition-table.gperf"
      {"\0000o\0000\232", 0x3071},
#line 475 "./uninorm/composition-table.gperf"
      {"\000\000m\000\003\001", 0x1e3f},
#line 636 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003#", 0x1ee5},
#line 128 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003\002", 0x0125},
#line 447 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003\007", 0x1e23},
#line 477 "./uninorm/composition-table.gperf"
      {"\000\000m\000\003\007", 0x1e41},
#line 753 "./uninorm/composition-table.gperf"
      {"\000\037h\000\003\001", 0x1f6c},
#line 751 "./uninorm/composition-table.gperf"
      {"\000\037h\000\003\000", 0x1f6a},
#line 755 "./uninorm/composition-table.gperf"
      {"\000\037h\000\003B", 0x1f6e},
#line 930 "./uninorm/composition-table.gperf"
      {"\0000u\0000\232", 0x3077},
#line 194 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\010", 0x0178},
#line 180 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\004", 0x016a},
#line 395 "./uninorm/composition-table.gperf"
      {"\000\015F\000\015W", 0x0d4c},
#line 654 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\011", 0x1ef7},
#line 211 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\014", 0x01d3},
#line 250 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\021", 0x0202},
#line 46 "./uninorm/composition-table.gperf"
      {"\000\000C\000\003'", 0x00c7},
#line 782 "./uninorm/composition-table.gperf"
      {"\000\037\"\000\003E", 0x1f92},
#line 804 "./uninorm/composition-table.gperf"
      {"\000\037h\000\003E", 0x1fa8},
#line 809 "./uninorm/composition-table.gperf"
      {"\000\037m\000\003E", 0x1fad},
#line 697 "./uninorm/composition-table.gperf"
      {"\000\037(\000\003\001", 0x1f2c},
#line 695 "./uninorm/composition-table.gperf"
      {"\000\037(\000\003\000", 0x1f2a},
#line 699 "./uninorm/composition-table.gperf"
      {"\000\037(\000\003B", 0x1f2e},
#line 490 "./uninorm/composition-table.gperf"
      {"\000\000\325\000\003\010", 0x1e4e},
#line 255 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003\021", 0x0207},
#line 706 "./uninorm/composition-table.gperf"
      {"\000\0371\000\003\001", 0x1f35},
#line 704 "./uninorm/composition-table.gperf"
      {"\000\0371\000\003\000", 0x1f33},
#line 708 "./uninorm/composition-table.gperf"
      {"\000\0371\000\003B", 0x1f37},
#line 291 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\004", 0x0233},
#line 962 "./uninorm/composition-table.gperf"
      {"\0000\246\0000\231", 0x30f4},
#line 437 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003-", 0x1e19},
#line 265 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003\017", 0x0211},
#line 788 "./uninorm/composition-table.gperf"
      {"\000\037(\000\003E", 0x1f98},
#line 451 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003\010", 0x1e27},
#line 45 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003\012", 0x00c5},
#line 655 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\003", 0x1ef8},
#line 591 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003#", 0x1eb8},
#line 542 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003\001", 0x1e82},
#line 540 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003\000", 0x1e80},
#line 160 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003\001", 0x0154},
#line 190 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003\002", 0x0174},
#line 546 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003\007", 0x1e86},
#line 912 "./uninorm/composition-table.gperf"
      {"\0000O\0000\231", 0x3050},
#line 500 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003\007", 0x1e58},
#line 878 "./uninorm/composition-table.gperf"
      {"\000\"C\000\0038", 0x2244},
#line 872 "./uninorm/composition-table.gperf"
      {"\000\"\003\000\0038", 0x2204},
#line 873 "./uninorm/composition-table.gperf"
      {"\000\"\010\000\0038", 0x2209},
#line 893 "./uninorm/composition-table.gperf"
      {"\000\"{\000\0038", 0x2281},
#line 177 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003\014", 0x0165},
#line 611 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003#", 0x1ecc},
#line 263 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\021", 0x020f},
#line 468 "./uninorm/composition-table.gperf"
      {"\000\0366\000\003\004", 0x1e38},
#line 238 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\001", 0x01f4},
#line 908 "./uninorm/composition-table.gperf"
      {"\000\"\264\000\0038", 0x22ec},
#line 906 "./uninorm/composition-table.gperf"
      {"\000\"\262\000\0038", 0x22ea},
#line 119 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\002", 0x011c},
#line 123 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\007", 0x0120},
#line 288 "./uninorm/composition-table.gperf"
      {"\000\002.\000\003\004", 0x0230},
#line 877 "./uninorm/composition-table.gperf"
      {"\000\"<\000\0038", 0x2241},
#line 271 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\021", 0x0217},
#line 905 "./uninorm/composition-table.gperf"
      {"\000\"\222\000\0038", 0x22e3},
#line 457 "./uninorm/composition-table.gperf"
      {"\000\000i\000\0030", 0x1e2d},
#line 898 "./uninorm/composition-table.gperf"
      {"\000\"\242\000\0038", 0x22ac},
#line 165 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003\014", 0x0159},
#line 915 "./uninorm/composition-table.gperf"
      {"\0000U\0000\231", 0x3056},
#line 531 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003-", 0x1e77},
#line 936 "./uninorm/composition-table.gperf"
      {"\0000\235\0000\231", 0x309e},
#line 331 "./uninorm/composition-table.gperf"
      {"\000\004t\000\003\017", 0x0476},
#line 882 "./uninorm/composition-table.gperf"
      {"\000\"a\000\0038", 0x2262},
#line 440 "./uninorm/composition-table.gperf"
      {"\000\002(\000\003\006", 0x1e1c},
#line 544 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003\010", 0x1e84},
#line 635 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003#", 0x1ee4},
#line 148 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003\001", 0x0143},
#line 240 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003\000", 0x01f8},
#line 153 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003\014", 0x0148},
#line 375 "./uninorm/composition-table.gperf"
      {"\000\011(\000\011<", 0x0929},
#line 480 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003\007", 0x1e44},
#line 897 "./uninorm/composition-table.gperf"
      {"\000\"\207\000\0038", 0x2289},
#line 185 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\012", 0x016f},
#line 896 "./uninorm/composition-table.gperf"
      {"\000\"\206\000\0038", 0x2288},
#line 456 "./uninorm/composition-table.gperf"
      {"\000\000I\000\0030", 0x1e2c},
#line 895 "./uninorm/composition-table.gperf"
      {"\000\"\203\000\0038", 0x2285},
#line 108 "./uninorm/composition-table.gperf"
      {"\000\000d\000\003\014", 0x010f},
#line 543 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\001", 0x1e83},
#line 541 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\000", 0x1e81},
#line 523 "./uninorm/composition-table.gperf"
      {"\000\000t\000\0031", 0x1e6f},
#line 191 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\002", 0x0175},
#line 547 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\007", 0x1e87},
#line 652 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003#", 0x1ef5},
#line 890 "./uninorm/composition-table.gperf"
      {"\000\"v\000\0038", 0x2278},
#line 297 "./uninorm/composition-table.gperf"
      {"\000\003\237\000\003\001", 0x038c},
#line 863 "./uninorm/composition-table.gperf"
      {"\000\003\237\000\003\000", 0x1ff8},
#line 239 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\001", 0x01f5},
#line 254 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003\021", 0x0206},
#line 653 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\011", 0x1ef6},
#line 120 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\002", 0x011d},
#line 124 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\007", 0x0121},
#line 343 "./uninorm/composition-table.gperf"
      {"\000\004\026\000\003\010", 0x04dc},
#line 902 "./uninorm/composition-table.gperf"
      {"\000\"|\000\0038", 0x22e0},
#line 436 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003-", 0x1e18},
#line 507 "./uninorm/composition-table.gperf"
      {"\000\000r\000\0031", 0x1e5f},
#line 276 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003\014", 0x021e},
#line 494 "./uninorm/composition-table.gperf"
      {"\000\001L\000\003\001", 0x1e52},
#line 492 "./uninorm/composition-table.gperf"
      {"\000\001L\000\003\000", 0x1e50},
#line 443 "./uninorm/composition-table.gperf"
      {"\000\000f\000\003\007", 0x1e1f},
#line 521 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003#", 0x1e6d},
#line 262 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\021", 0x020e},
#line 894 "./uninorm/composition-table.gperf"
      {"\000\"\202\000\0038", 0x2284},
#line 142 "./uninorm/composition-table.gperf"
      {"\000\000L\000\003\001", 0x0139},
#line 290 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003\004", 0x0232},
#line 803 "./uninorm/composition-table.gperf"
      {"\000\037g\000\003E", 0x1fa7},
#line 485 "./uninorm/composition-table.gperf"
      {"\000\000n\000\0031", 0x1e49},
#line 281 "./uninorm/composition-table.gperf"
      {"\000\000e\000\003'", 0x0229},
#line 927 "./uninorm/composition-table.gperf"
      {"\0000r\0000\231", 0x3073},
#line 121 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\006", 0x011e},
#line 313 "./uninorm/composition-table.gperf"
      {"\000\003\322\000\003\001", 0x03d3},
#line 545 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\010", 0x1e85},
#line 439 "./uninorm/composition-table.gperf"
      {"\000\000e\000\0030", 0x1e1b},
#line 802 "./uninorm/composition-table.gperf"
      {"\000\037f\000\003E", 0x1fa6},
#line 427 "./uninorm/composition-table.gperf"
      {"\000\000d\000\0031", 0x1e0f},
#line 503 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003#", 0x1e5b},
#line 418 "./uninorm/composition-table.gperf"
      {"\000\000B\000\0031", 0x1e06},
#line 284 "./uninorm/composition-table.gperf"
      {"\000\000\325\000\003\004", 0x022c},
#line 495 "./uninorm/composition-table.gperf"
      {"\000\001M\000\003\001", 0x1e53},
#line 493 "./uninorm/composition-table.gperf"
      {"\000\001M\000\003\000", 0x1e51},
#line 903 "./uninorm/composition-table.gperf"
      {"\000\"}\000\0038", 0x22e1},
#line 333 "./uninorm/composition-table.gperf"
      {"\000\004\026\000\003\006", 0x04c1},
#line 928 "./uninorm/composition-table.gperf"
      {"\0000r\0000\232", 0x3074},
#line 270 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\021", 0x0216},
#line 474 "./uninorm/composition-table.gperf"
      {"\000\000M\000\003\001", 0x1e3e},
#line 55 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003\003", 0x00d1},
#line 483 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003#", 0x1e47},
#line 922 "./uninorm/composition-table.gperf"
      {"\0000d\0000\231", 0x3065},
#line 476 "./uninorm/composition-table.gperf"
      {"\000\000M\000\003\007", 0x1e40},
#line 530 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003-", 0x1e76},
#line 277 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003\014", 0x021f},
#line 364 "./uninorm/composition-table.gperf"
      {"\000\004G\000\003\010", 0x04f5},
#line 518 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003\007", 0x1e6a},
#line 920 "./uninorm/composition-table.gperf"
      {"\0000_\0000\231", 0x3060},
#line 425 "./uninorm/composition-table.gperf"
      {"\000\000d\000\003#", 0x1e0d},
#line 887 "./uninorm/composition-table.gperf"
      {"\000\"e\000\0038", 0x2271},
#line 416 "./uninorm/composition-table.gperf"
      {"\000\000B\000\003#", 0x1e04},
#line 264 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003\017", 0x0210},
#line 844 "./uninorm/composition-table.gperf"
      {"\000\037\376\000\003\001", 0x1fde},
#line 843 "./uninorm/composition-table.gperf"
      {"\000\037\376\000\003\000", 0x1fdd},
#line 845 "./uninorm/composition-table.gperf"
      {"\000\037\376\000\003B", 0x1fdf},
#line 274 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003&", 0x021a},
#line 184 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\012", 0x016e},
#line 314 "./uninorm/composition-table.gperf"
      {"\000\003\322\000\003\010", 0x03d4},
#line 310 "./uninorm/composition-table.gperf"
      {"\000\003\277\000\003\001", 0x03cc},
#line 761 "./uninorm/composition-table.gperf"
      {"\000\003\277\000\003\000", 0x1f78},
#line 723 "./uninorm/composition-table.gperf"
      {"\000\003\237\000\003\023", 0x1f48},
#line 529 "./uninorm/composition-table.gperf"
      {"\000\000u\000\0030", 0x1e75},
#line 919 "./uninorm/composition-table.gperf"
      {"\0000]\0000\231", 0x305e},
#line 122 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\006", 0x011f},
#line 306 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\001", 0x03af},
#line 760 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\000", 0x1f76},
#line 838 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003B", 0x1fd6},
#line 448 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003#", 0x1e24},
#line 833 "./uninorm/composition-table.gperf"
      {"\000\037\277\000\003\001", 0x1fce},
#line 832 "./uninorm/composition-table.gperf"
      {"\000\037\277\000\003\000", 0x1fcd},
#line 834 "./uninorm/composition-table.gperf"
      {"\000\037\277\000\003B", 0x1fcf},
#line 917 "./uninorm/composition-table.gperf"
      {"\0000Y\0000\231", 0x305a},
#line 319 "./uninorm/composition-table.gperf"
      {"\000\004\032\000\003\001", 0x040c},
#line 565 "./uninorm/composition-table.gperf"
      {"\000\000y\000\003\012", 0x1e99},
#line 885 "./uninorm/composition-table.gperf"
      {"\000\000>\000\0038", 0x226f},
#line 435 "./uninorm/composition-table.gperf"
      {"\000\001\023\000\003\001", 0x1e17},
#line 433 "./uninorm/composition-table.gperf"
      {"\000\001\023\000\003\000", 0x1e15},
#line 525 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003-", 0x1e71},
#line 651 "./uninorm/composition-table.gperf"
      {"\000\000Y\000\003#", 0x1ef4},
#line 345 "./uninorm/composition-table.gperf"
      {"\000\004\027\000\003\010", 0x04de},
#line 776 "./uninorm/composition-table.gperf"
      {"\000\037\014\000\003E", 0x1f8c},
#line 562 "./uninorm/composition-table.gperf"
      {"\000\000h\000\0031", 0x1e96},
#line 941 "./uninorm/composition-table.gperf"
      {"\0000\263\0000\231", 0x30b4},
#line 164 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003\014", 0x0158},
#line 956 "./uninorm/composition-table.gperf"
      {"\0000\325\0000\231", 0x30d6},
#line 779 "./uninorm/composition-table.gperf"
      {"\000\037\017\000\003E", 0x1f8f},
#line 267 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003\021", 0x0213},
#line 305 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003\001", 0x03ae},
#line 759 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003\000", 0x1f74},
#line 827 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003B", 0x1fc6},
#line 280 "./uninorm/composition-table.gperf"
      {"\000\000E\000\003'", 0x0228},
#line 626 "./uninorm/composition-table.gperf"
      {"\000\001\241\000\003\001", 0x1edb},
#line 628 "./uninorm/composition-table.gperf"
      {"\000\001\241\000\003\000", 0x1edd},
#line 444 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\004", 0x1e20},
#line 924 "./uninorm/composition-table.gperf"
      {"\0000h\0000\231", 0x3069},
#line 438 "./uninorm/composition-table.gperf"
      {"\000\000E\000\0030", 0x1e1a},
#line 227 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003\014", 0x01e6},
#line 308 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\010", 0x03ca},
#line 957 "./uninorm/composition-table.gperf"
      {"\0000\325\0000\232", 0x30d7},
#line 825 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003E", 0x1fc3},
#line 960 "./uninorm/composition-table.gperf"
      {"\0000\333\0000\231", 0x30dc},
#line 449 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003#", 0x1e25},
#line 479 "./uninorm/composition-table.gperf"
      {"\000\000m\000\003#", 0x1e43},
#line 487 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003-", 0x1e4b},
#line 937 "./uninorm/composition-table.gperf"
      {"\0000\253\0000\231", 0x30ac},
#line 303 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\001", 0x03ac},
#line 757 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\000", 0x1f70},
#line 817 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003B", 0x1fb6},
#line 352 "./uninorm/composition-table.gperf"
      {"\000\004>\000\003\010", 0x04e7},
#line 293 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\001", 0x0386},
#line 821 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\000", 0x1fba},
#line 431 "./uninorm/composition-table.gperf"
      {"\000\000d\000\003-", 0x1e13},
#line 606 "./uninorm/composition-table.gperf"
      {"\000\036\271\000\003\002", 0x1ec7},
#line 778 "./uninorm/composition-table.gperf"
      {"\000\037\016\000\003E", 0x1f8e},
#line 961 "./uninorm/composition-table.gperf"
      {"\0000\333\0000\232", 0x30dd},
#line 362 "./uninorm/composition-table.gperf"
      {"\000\004C\000\003\013", 0x04f3},
#line 152 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003\014", 0x0147},
#line 815 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003E", 0x1fb3},
#line 879 "./uninorm/composition-table.gperf"
      {"\000\"E\000\0038", 0x2247},
#line 717 "./uninorm/composition-table.gperf"
      {"\000\003\277\000\003\023", 0x1f40},
#line 506 "./uninorm/composition-table.gperf"
      {"\000\000R\000\0031", 0x1e5e},
#line 822 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003E", 0x1fbc},
#line 356 "./uninorm/composition-table.gperf"
      {"\000\004M\000\003\010", 0x04ed},
#line 724 "./uninorm/composition-table.gperf"
      {"\000\003\237\000\003\024", 0x1f49},
#line 167 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003\001", 0x015b},
#line 701 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\023", 0x1f30},
#line 835 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\006", 0x1fd0},
#line 169 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003\002", 0x015d},
#line 509 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003\007", 0x1e61},
#line 977 "./uninorm/composition-table.gperf"
      {"\001\024\271\001\024\275", 0x114be},
#line 528 "./uninorm/composition-table.gperf"
      {"\000\000U\000\0030", 0x1e74},
#line 916 "./uninorm/composition-table.gperf"
      {"\0000W\0000\231", 0x3058},
#line 317 "./uninorm/composition-table.gperf"
      {"\000\004\023\000\003\001", 0x0403},
#line 445 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\004", 0x1e21},
#line 460 "./uninorm/composition-table.gperf"
      {"\000\000K\000\003\001", 0x1e30},
#line 770 "./uninorm/composition-table.gperf"
      {"\000\037\006\000\003E", 0x1f86},
#line 228 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003\014", 0x01e7},
#line 273 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003&", 0x0219},
#line 548 "./uninorm/composition-table.gperf"
      {"\000\000W\000\003#", 0x1e88},
#line 244 "./uninorm/composition-table.gperf"
      {"\000\000\306\000\003\001", 0x01fc},
#line 502 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003#", 0x1e5a},
#line 580 "./uninorm/composition-table.gperf"
      {"\000\036\241\000\003\002", 0x1ead},
#line 311 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\001", 0x03cd},
#line 762 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\000", 0x1f7a},
#line 851 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003B", 0x1fe6},
#line 143 "./uninorm/composition-table.gperf"
      {"\000\000l\000\003\001", 0x013a},
#line 242 "./uninorm/composition-table.gperf"
      {"\000\000\305\000\003\001", 0x01fa},
#line 166 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003\001", 0x015a},
#line 685 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003\023", 0x1f20},
#line 632 "./uninorm/composition-table.gperf"
      {"\000\001\241\000\003\003", 0x1ee1},
#line 168 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003\002", 0x015c},
#line 508 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003\007", 0x1e60},
#line 146 "./uninorm/composition-table.gperf"
      {"\000\000L\000\003\014", 0x013d},
#line 484 "./uninorm/composition-table.gperf"
      {"\000\000N\000\0031", 0x1e48},
#line 296 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\001", 0x038a},
#line 842 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\000", 0x1fda},
#line 512 "./uninorm/composition-table.gperf"
      {"\000\001Z\000\003\007", 0x1e64},
#line 828 "./uninorm/composition-table.gperf"
      {"\000\037\306\000\003E", 0x1fc7},
#line 195 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\003\001", 0x0179},
#line 175 "./uninorm/composition-table.gperf"
      {"\000\000t\000\003'", 0x0163},
#line 272 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003&", 0x0218},
#line 556 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\003\002", 0x1e90},
#line 197 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\003\007", 0x017b},
#line 808 "./uninorm/composition-table.gperf"
      {"\000\037l\000\003E", 0x1fac},
      {""},
#line 100 "./uninorm/composition-table.gperf"
      {"\000\000c\000\003\001", 0x0107},
#line 657 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\023", 0x1f00},
#line 812 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\006", 0x1fb0},
#line 102 "./uninorm/composition-table.gperf"
      {"\000\000c\000\003\002", 0x0109},
#line 104 "./uninorm/composition-table.gperf"
      {"\000\000c\000\003\007", 0x010b},
#line 665 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\023", 0x1f08},
#line 819 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\006", 0x1fb8},
      {""},
#line 482 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003#", 0x1e46},
#line 163 "./uninorm/composition-table.gperf"
      {"\000\000r\000\003'", 0x0157},
#line 309 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\010", 0x03cb},
#line 196 "./uninorm/composition-table.gperf"
      {"\000\000z\000\003\001", 0x017a},
#line 176 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003\014", 0x0164},
#line 422 "./uninorm/composition-table.gperf"
      {"\000\000D\000\003\007", 0x1e0a},
#line 557 "./uninorm/composition-table.gperf"
      {"\000\000z\000\003\002", 0x1e91},
#line 198 "./uninorm/composition-table.gperf"
      {"\000\000z\000\003\007", 0x017c},
      {""},
#line 718 "./uninorm/composition-table.gperf"
      {"\000\003\277\000\003\024", 0x1f41},
#line 799 "./uninorm/composition-table.gperf"
      {"\000\037c\000\003E", 0x1fa3},
#line 549 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003#", 0x1e89},
#line 151 "./uninorm/composition-table.gperf"
      {"\000\000n\000\003'", 0x0146},
#line 159 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\013", 0x0151},
#line 301 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\010", 0x03aa},
#line 702 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\024", 0x1f31},
#line 470 "./uninorm/composition-table.gperf"
      {"\000\000L\000\0031", 0x1e3a},
      {""},
#line 640 "./uninorm/composition-table.gperf"
      {"\000\001\260\000\003\001", 0x1ee9},
#line 642 "./uninorm/composition-table.gperf"
      {"\000\001\260\000\003\000", 0x1eeb},
#line 429 "./uninorm/composition-table.gperf"
      {"\000\000d\000\003'", 0x1e11},
#line 923 "./uninorm/composition-table.gperf"
      {"\0000f\0000\231", 0x3067},
#line 187 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\013", 0x0171},
#line 836 "./uninorm/composition-table.gperf"
      {"\000\003\271\000\003\004", 0x1fd1},
#line 266 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003\021", 0x0212},
#line 625 "./uninorm/composition-table.gperf"
      {"\000\001\240\000\003\001", 0x1eda},
#line 627 "./uninorm/composition-table.gperf"
      {"\000\001\240\000\003\000", 0x1edc},
#line 590 "./uninorm/composition-table.gperf"
      {"\000\036\241\000\003\006", 0x1eb7},
#line 318 "./uninorm/composition-table.gperf"
      {"\000\004\006\000\003\010", 0x0407},
#line 888 "./uninorm/composition-table.gperf"
      {"\000\"r\000\0038", 0x2274},
#line 729 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\023", 0x1f50},
#line 846 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\006", 0x1fe0},
#line 639 "./uninorm/composition-table.gperf"
      {"\000\001\257\000\003\001", 0x1ee8},
#line 641 "./uninorm/composition-table.gperf"
      {"\000\001\257\000\003\000", 0x1eea},
      {""},
#line 630 "./uninorm/composition-table.gperf"
      {"\000\001\241\000\003\011", 0x1edf},
#line 466 "./uninorm/composition-table.gperf"
      {"\000\000L\000\003#", 0x1e36},
#line 517 "./uninorm/composition-table.gperf"
      {"\000\036c\000\003\007", 0x1e69},
#line 686 "./uninorm/composition-table.gperf"
      {"\000\003\267\000\003\024", 0x1f21},
#line 452 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003'", 0x1e28},
#line 954 "./uninorm/composition-table.gperf"
      {"\0000\322\0000\231", 0x30d3},
#line 522 "./uninorm/composition-table.gperf"
      {"\000\000T\000\0031", 0x1e6e},
#line 709 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\023", 0x1f38},
#line 840 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\006", 0x1fd8},
#line 856 "./uninorm/composition-table.gperf"
      {"\000\003\241\000\003\024", 0x1fec},
#line 366 "./uninorm/composition-table.gperf"
      {"\000\004K\000\003\010", 0x04f9},
#line 874 "./uninorm/composition-table.gperf"
      {"\000\"\013\000\0038", 0x220c},
#line 235 "./uninorm/composition-table.gperf"
      {"\000\001\267\000\003\014", 0x01ee},
#line 886 "./uninorm/composition-table.gperf"
      {"\000\"d\000\0038", 0x2270},
      {""},
#line 911 "./uninorm/composition-table.gperf"
      {"\0000M\0000\231", 0x304e},
#line 415 "./uninorm/composition-table.gperf"
      {"\000\000b\000\003\007", 0x1e03},
#line 785 "./uninorm/composition-table.gperf"
      {"\000\037%\000\003E", 0x1f95},
      {""},
#line 955 "./uninorm/composition-table.gperf"
      {"\0000\322\0000\232", 0x30d4},
#line 394 "./uninorm/composition-table.gperf"
      {"\000\015G\000\015>", 0x0d4b},
#line 658 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\024", 0x1f01},
#line 478 "./uninorm/composition-table.gperf"
      {"\000\000M\000\003#", 0x1e42},
#line 304 "./uninorm/composition-table.gperf"
      {"\000\003\265\000\003\001", 0x03ad},
#line 758 "./uninorm/composition-table.gperf"
      {"\000\003\265\000\003\000", 0x1f72},
#line 666 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\024", 0x1f09},
#line 520 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003#", 0x1e6c},
#line 975 "./uninorm/composition-table.gperf"
      {"\001\024\271\001\024\272", 0x114bb},
#line 486 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003-", 0x1e4a},
#line 813 "./uninorm/composition-table.gperf"
      {"\000\003\261\000\003\004", 0x1fb1},
#line 798 "./uninorm/composition-table.gperf"
      {"\000\037b\000\003E", 0x1fa2},
#line 783 "./uninorm/composition-table.gperf"
      {"\000\037#\000\003E", 0x1f93},
#line 880 "./uninorm/composition-table.gperf"
      {"\000\"H\000\0038", 0x2249},
#line 820 "./uninorm/composition-table.gperf"
      {"\000\003\221\000\003\004", 0x1fb9},
#line 158 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\013", 0x0150},
#line 461 "./uninorm/composition-table.gperf"
      {"\000\000k\000\003\001", 0x1e31},
#line 795 "./uninorm/composition-table.gperf"
      {"\000\037/\000\003E", 0x1f9f},
#line 246 "./uninorm/composition-table.gperf"
      {"\000\000\330\000\003\001", 0x01fe},
#line 453 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003'", 0x1e29},
#line 947 "./uninorm/composition-table.gperf"
      {"\0000\277\0000\231", 0x30c0},
#line 579 "./uninorm/composition-table.gperf"
      {"\000\036\240\000\003\002", 0x1eac},
#line 646 "./uninorm/composition-table.gperf"
      {"\000\001\260\000\003\003", 0x1eef},
#line 300 "./uninorm/composition-table.gperf"
      {"\000\003\312\000\003\001", 0x0390},
#line 837 "./uninorm/composition-table.gperf"
      {"\000\003\312\000\003\000", 0x1fd2},
#line 839 "./uninorm/composition-table.gperf"
      {"\000\003\312\000\003B", 0x1fd7},
#line 944 "./uninorm/composition-table.gperf"
      {"\0000\271\0000\231", 0x30ba},
#line 597 "./uninorm/composition-table.gperf"
      {"\000\000\312\000\003\001", 0x1ebe},
#line 599 "./uninorm/composition-table.gperf"
      {"\000\000\312\000\003\000", 0x1ec0},
#line 631 "./uninorm/composition-table.gperf"
      {"\000\001\240\000\003\003", 0x1ee0},
#line 849 "./uninorm/composition-table.gperf"
      {"\000\003\301\000\003\023", 0x1fe4},
#line 408 "./uninorm/composition-table.gperf"
      {"\000\033<\000\0335", 0x1b3d},
#line 173 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003\014", 0x0161},
#line 564 "./uninorm/composition-table.gperf"
      {"\000\000w\000\003\012", 0x1e98},
#line 807 "./uninorm/composition-table.gperf"
      {"\000\037k\000\003E", 0x1fab},
#line 907 "./uninorm/composition-table.gperf"
      {"\000\"\263\000\0038", 0x22eb},
#line 645 "./uninorm/composition-table.gperf"
      {"\000\001\257\000\003\003", 0x1eee},
#line 186 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\013", 0x0170},
#line 516 "./uninorm/composition-table.gperf"
      {"\000\036b\000\003\007", 0x1e68},
#line 730 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\024", 0x1f51},
#line 299 "./uninorm/composition-table.gperf"
      {"\000\003\251\000\003\001", 0x038f},
#line 864 "./uninorm/composition-table.gperf"
      {"\000\003\251\000\003\000", 0x1ffa},
#line 229 "./uninorm/composition-table.gperf"
      {"\000\000K\000\003\014", 0x01e8},
#line 535 "./uninorm/composition-table.gperf"
      {"\000\001k\000\003\010", 0x1e7b},
#line 225 "./uninorm/composition-table.gperf"
      {"\000\000\306\000\003\004", 0x01e2},
#line 472 "./uninorm/composition-table.gperf"
      {"\000\000L\000\003-", 0x1e3c},
#line 407 "./uninorm/composition-table.gperf"
      {"\000\033:\000\0335", 0x1b3b},
#line 847 "./uninorm/composition-table.gperf"
      {"\000\003\305\000\003\004", 0x1fe1},
#line 403 "./uninorm/composition-table.gperf"
      {"\000\033\011\000\0335", 0x1b0a},
#line 943 "./uninorm/composition-table.gperf"
      {"\0000\267\0000\231", 0x30b8},
      {""},
#line 710 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\024", 0x1f39},
#line 865 "./uninorm/composition-table.gperf"
      {"\000\003\251\000\003E", 0x1ffc},
#line 147 "./uninorm/composition-table.gperf"
      {"\000\000l\000\003\014", 0x013e},
#line 551 "./uninorm/composition-table.gperf"
      {"\000\000x\000\003\007", 0x1e8b},
#line 172 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003\014", 0x0160},
#line 634 "./uninorm/composition-table.gperf"
      {"\000\001\241\000\003#", 0x1ee3},
#line 901 "./uninorm/composition-table.gperf"
      {"\000\"\253\000\0038", 0x22af},
#line 162 "./uninorm/composition-table.gperf"
      {"\000\000R\000\003'", 0x0156},
#line 841 "./uninorm/composition-table.gperf"
      {"\000\003\231\000\003\004", 0x1fd9},
#line 713 "./uninorm/composition-table.gperf"
      {"\000\0378\000\003\001", 0x1f3c},
#line 711 "./uninorm/composition-table.gperf"
      {"\000\0378\000\003\000", 0x1f3a},
#line 715 "./uninorm/composition-table.gperf"
      {"\000\0378\000\003B", 0x1f3e},
#line 298 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\001", 0x038e},
#line 855 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\000", 0x1fea},
#line 673 "./uninorm/composition-table.gperf"
      {"\000\003\265\000\003\023", 0x1f10},
#line 199 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\003\014", 0x017d},
#line 940 "./uninorm/composition-table.gperf"
      {"\0000\261\0000\231", 0x30b2},
#line 683 "./uninorm/composition-table.gperf"
      {"\000\037\030\000\003\001", 0x1f1c},
#line 681 "./uninorm/composition-table.gperf"
      {"\000\037\030\000\003\000", 0x1f1a},
#line 524 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003-", 0x1e70},
#line 125 "./uninorm/composition-table.gperf"
      {"\000\000G\000\003'", 0x0122},
#line 513 "./uninorm/composition-table.gperf"
      {"\000\001[\000\003\007", 0x1e65},
#line 106 "./uninorm/composition-table.gperf"
      {"\000\000c\000\003\014", 0x010d},
#line 312 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003\001", 0x03ce},
#line 763 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003\000", 0x1f7c},
#line 861 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003B", 0x1ff6},
#line 402 "./uninorm/composition-table.gperf"
      {"\000\033\007\000\0335", 0x1b08},
#line 359 "./uninorm/composition-table.gperf"
      {"\000\004#\000\003\010", 0x04f0},
#line 464 "./uninorm/composition-table.gperf"
      {"\000\000K\000\0031", 0x1e34},
#line 589 "./uninorm/composition-table.gperf"
      {"\000\036\240\000\003\006", 0x1eb6},
#line 644 "./uninorm/composition-table.gperf"
      {"\000\001\260\000\003\011", 0x1eed},
#line 107 "./uninorm/composition-table.gperf"
      {"\000\000D\000\003\014", 0x010e},
#line 553 "./uninorm/composition-table.gperf"
      {"\000\000x\000\003\010", 0x1e8d},
#line 200 "./uninorm/composition-table.gperf"
      {"\000\000z\000\003\014", 0x017e},
#line 603 "./uninorm/composition-table.gperf"
      {"\000\000\312\000\003\003", 0x1ec4},
#line 859 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003E", 0x1ff3},
#line 458 "./uninorm/composition-table.gperf"
      {"\000\000\317\000\003\001", 0x1e2e},
#line 629 "./uninorm/composition-table.gperf"
      {"\000\001\240\000\003\011", 0x1ede},
#line 511 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003#", 0x1e63},
#line 471 "./uninorm/composition-table.gperf"
      {"\000\000l\000\0031", 0x1e3b},
#line 150 "./uninorm/composition-table.gperf"
      {"\000\000N\000\003'", 0x0145},
#line 910 "./uninorm/composition-table.gperf"
      {"\0000K\0000\231", 0x304c},
#line 850 "./uninorm/composition-table.gperf"
      {"\000\003\301\000\003\024", 0x1fe5},
#line 536 "./uninorm/composition-table.gperf"
      {"\000\000V\000\003\003", 0x1e7c},
#line 643 "./uninorm/composition-table.gperf"
      {"\000\001\257\000\003\011", 0x1eec},
#line 302 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\010", 0x03ab},
#line 950 "./uninorm/composition-table.gperf"
      {"\0000\306\0000\231", 0x30c7},
#line 368 "./uninorm/composition-table.gperf"
      {"\000\006'\000\006T", 0x0623},
#line 462 "./uninorm/composition-table.gperf"
      {"\000\000K\000\003#", 0x1e32},
      {""},
#line 749 "./uninorm/composition-table.gperf"
      {"\000\003\251\000\003\023", 0x1f68},
#line 341 "./uninorm/composition-table.gperf"
      {"\000\004\330\000\003\010", 0x04da},
#line 560 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\0031", 0x1e94},
#line 504 "./uninorm/composition-table.gperf"
      {"\000\036Z\000\003\004", 0x1e5c},
#line 914 "./uninorm/composition-table.gperf"
      {"\0000S\0000\231", 0x3054},
#line 329 "./uninorm/composition-table.gperf"
      {"\000\0048\000\003\000", 0x045d},
#line 321 "./uninorm/composition-table.gperf"
      {"\000\004#\000\003\006", 0x040e},
#line 615 "./uninorm/composition-table.gperf"
      {"\000\000\324\000\003\001", 0x1ed0},
#line 617 "./uninorm/composition-table.gperf"
      {"\000\000\324\000\003\000", 0x1ed2},
#line 467 "./uninorm/composition-table.gperf"
      {"\000\000l\000\003#", 0x1e37},
#line 126 "./uninorm/composition-table.gperf"
      {"\000\000g\000\003'", 0x0123},
#line 510 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003#", 0x1e62},
#line 527 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003$", 0x1e73},
#line 320 "./uninorm/composition-table.gperf"
      {"\000\004\030\000\003\000", 0x040d},
      {""},
#line 598 "./uninorm/composition-table.gperf"
      {"\000\000\352\000\003\001", 0x1ebf},
#line 600 "./uninorm/composition-table.gperf"
      {"\000\000\352\000\003\000", 0x1ec1},
      {""},
#line 426 "./uninorm/composition-table.gperf"
      {"\000\000D\000\0031", 0x1e0e},
#line 327 "./uninorm/composition-table.gperf"
      {"\000\004V\000\003\010", 0x0457},
#line 561 "./uninorm/composition-table.gperf"
      {"\000\000z\000\0031", 0x1e95},
#line 378 "./uninorm/composition-table.gperf"
      {"\000\011\307\000\011\276", 0x09cb},
#line 558 "./uninorm/composition-table.gperf"
      {"\000\000Z\000\003#", 0x1e92},
#line 289 "./uninorm/composition-table.gperf"
      {"\000\002/\000\003\004", 0x0231},
#line 853 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\006", 0x1fe8},
#line 972 "./uninorm/composition-table.gperf"
      {"\001\0212\001\021'", 0x1112f},
#line 144 "./uninorm/composition-table.gperf"
      {"\000\000L\000\003'", 0x013b},
#line 674 "./uninorm/composition-table.gperf"
      {"\000\003\265\000\003\024", 0x1f11},
#line 371 "./uninorm/composition-table.gperf"
      {"\000\006J\000\006T", 0x0626},
#line 891 "./uninorm/composition-table.gperf"
      {"\000\"w\000\0038", 0x2279},
#line 690 "./uninorm/composition-table.gperf"
      {"\000\037!\000\003\001", 0x1f25},
#line 688 "./uninorm/composition-table.gperf"
      {"\000\037!\000\003\000", 0x1f23},
#line 692 "./uninorm/composition-table.gperf"
      {"\000\037!\000\003B", 0x1f27},
#line 381 "./uninorm/composition-table.gperf"
      {"\000\013G\000\013>", 0x0b4b},
#line 741 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003\023", 0x1f60},
#line 295 "./uninorm/composition-table.gperf"
      {"\000\003\227\000\003\001", 0x0389},
#line 830 "./uninorm/composition-table.gperf"
      {"\000\003\227\000\003\000", 0x1fca},
#line 350 "./uninorm/composition-table.gperf"
      {"\000\0048\000\003\010", 0x04e5},
#line 424 "./uninorm/composition-table.gperf"
      {"\000\000D\000\003#", 0x1e0c},
#line 968 "./uninorm/composition-table.gperf"
      {"\001\020\231\001\020\272", 0x1109a},
#line 559 "./uninorm/composition-table.gperf"
      {"\000\000z\000\003#", 0x1e93},
      {""},
#line 781 "./uninorm/composition-table.gperf"
      {"\000\037!\000\003E", 0x1f91},
#line 601 "./uninorm/composition-table.gperf"
      {"\000\000\312\000\003\011", 0x1ec2},
      {""},
#line 349 "./uninorm/composition-table.gperf"
      {"\000\004\030\000\003\010", 0x04e4},
#line 401 "./uninorm/composition-table.gperf"
      {"\000\033\005\000\0335", 0x1b06},
#line 831 "./uninorm/composition-table.gperf"
      {"\000\003\227\000\003E", 0x1fcc},
#line 978 "./uninorm/composition-table.gperf"
      {"\001\025\270\001\025\257", 0x115ba},
#line 405 "./uninorm/composition-table.gperf"
      {"\000\033\015\000\0335", 0x1b0e},
#line 230 "./uninorm/composition-table.gperf"
      {"\000\000k\000\003\014", 0x01e9},
#line 174 "./uninorm/composition-table.gperf"
      {"\000\000T\000\003'", 0x0162},
#line 496 "./uninorm/composition-table.gperf"
      {"\000\000P\000\003\001", 0x1e54},
#line 410 "./uninorm/composition-table.gperf"
      {"\000\033?\000\0335", 0x1b41},
      {""},
#line 648 "./uninorm/composition-table.gperf"
      {"\000\001\260\000\003#", 0x1ef1},
#line 498 "./uninorm/composition-table.gperf"
      {"\000\000P\000\003\007", 0x1e56},
#line 948 "./uninorm/composition-table.gperf"
      {"\0000\301\0000\231", 0x30c2},
#line 733 "./uninorm/composition-table.gperf"
      {"\000\037P\000\003\001", 0x1f54},
#line 731 "./uninorm/composition-table.gperf"
      {"\000\037P\000\003\000", 0x1f52},
#line 735 "./uninorm/composition-table.gperf"
      {"\000\037P\000\003B", 0x1f56},
      {""},
#line 633 "./uninorm/composition-table.gperf"
      {"\000\001\240\000\003#", 0x1ee2},
#line 750 "./uninorm/composition-table.gperf"
      {"\000\003\251\000\003\024", 0x1f69},
#line 419 "./uninorm/composition-table.gperf"
      {"\000\000b\000\0031", 0x1e07},
#line 621 "./uninorm/composition-table.gperf"
      {"\000\000\324\000\003\003", 0x1ed6},
#line 323 "./uninorm/composition-table.gperf"
      {"\000\0048\000\003\006", 0x0439},
#line 868 "./uninorm/composition-table.gperf"
      {"\000!\224\000\0038", 0x21ae},
#line 939 "./uninorm/composition-table.gperf"
      {"\0000\257\0000\231", 0x30b0},
#line 647 "./uninorm/composition-table.gperf"
      {"\000\001\257\000\003#", 0x1ef0},
#line 514 "./uninorm/composition-table.gperf"
      {"\000\001`\000\003\007", 0x1e66},
#line 866 "./uninorm/composition-table.gperf"
      {"\000!\220\000\0038", 0x219a},
#line 473 "./uninorm/composition-table.gperf"
      {"\000\000l\000\003-", 0x1e3d},
#line 604 "./uninorm/composition-table.gperf"
      {"\000\000\352\000\003\003", 0x1ec5},
#line 322 "./uninorm/composition-table.gperf"
      {"\000\004\030\000\003\006", 0x0419},
#line 883 "./uninorm/composition-table.gperf"
      {"\000\"M\000\0038", 0x226d},
#line 357 "./uninorm/composition-table.gperf"
      {"\000\004#\000\003\004", 0x04ee},
      {""},
#line 745 "./uninorm/composition-table.gperf"
      {"\000\037`\000\003\001", 0x1f64},
#line 743 "./uninorm/composition-table.gperf"
      {"\000\037`\000\003\000", 0x1f62},
#line 747 "./uninorm/composition-table.gperf"
      {"\000\037`\000\003B", 0x1f66},
#line 526 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003$", 0x1e72},
#line 616 "./uninorm/composition-table.gperf"
      {"\000\000\364\000\003\001", 0x1ed1},
#line 618 "./uninorm/composition-table.gperf"
      {"\000\000\364\000\003\000", 0x1ed3},
#line 417 "./uninorm/composition-table.gperf"
      {"\000\000b\000\003#", 0x1e05},
#line 867 "./uninorm/composition-table.gperf"
      {"\000!\222\000\0038", 0x219b},
#line 737 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\024", 0x1f59},
#line 465 "./uninorm/composition-table.gperf"
      {"\000\000k\000\0031", 0x1e35},
      {""},
#line 382 "./uninorm/composition-table.gperf"
      {"\000\013G\000\013W", 0x0b4c},
#line 796 "./uninorm/composition-table.gperf"
      {"\000\037`\000\003E", 0x1fa0},
      {""},
#line 942 "./uninorm/composition-table.gperf"
      {"\0000\265\0000\231", 0x30b6},
      {""},
#line 854 "./uninorm/composition-table.gperf"
      {"\000\003\245\000\003\004", 0x1fe9},
      {""},
#line 693 "./uninorm/composition-table.gperf"
      {"\000\003\227\000\003\023", 0x1f28},
#line 742 "./uninorm/composition-table.gperf"
      {"\000\003\311\000\003\024", 0x1f61},
#line 869 "./uninorm/composition-table.gperf"
      {"\000!\320\000\0038", 0x21cd},
#line 387 "./uninorm/composition-table.gperf"
      {"\000\014F\000\014V", 0x0c48},
      {""},
#line 430 "./uninorm/composition-table.gperf"
      {"\000\000D\000\003-", 0x1e12},
#line 958 "./uninorm/composition-table.gperf"
      {"\0000\330\0000\231", 0x30d9},
#line 294 "./uninorm/composition-table.gperf"
      {"\000\003\225\000\003\001", 0x0388},
#line 829 "./uninorm/composition-table.gperf"
      {"\000\003\225\000\003\000", 0x1fc8},
#line 572 "./uninorm/composition-table.gperf"
      {"\000\000\342\000\003\001", 0x1ea5},
#line 574 "./uninorm/composition-table.gperf"
      {"\000\000\342\000\003\000", 0x1ea7},
#line 463 "./uninorm/composition-table.gperf"
      {"\000\000k\000\003#", 0x1e33},
#line 307 "./uninorm/composition-table.gperf"
      {"\000\003\313\000\003\001", 0x03b0},
#line 848 "./uninorm/composition-table.gperf"
      {"\000\003\313\000\003\000", 0x1fe2},
#line 852 "./uninorm/composition-table.gperf"
      {"\000\003\313\000\003B", 0x1fe7},
#line 571 "./uninorm/composition-table.gperf"
      {"\000\000\302\000\003\001", 0x1ea4},
#line 573 "./uninorm/composition-table.gperf"
      {"\000\000\302\000\003\000", 0x1ea6},
#line 292 "./uninorm/composition-table.gperf"
      {"\000\000\250\000\003\001", 0x0385},
#line 857 "./uninorm/composition-table.gperf"
      {"\000\000\250\000\003\000", 0x1fed},
#line 823 "./uninorm/composition-table.gperf"
      {"\000\000\250\000\003B", 0x1fc1},
#line 959 "./uninorm/composition-table.gperf"
      {"\0000\330\0000\232", 0x30da},
#line 734 "./uninorm/composition-table.gperf"
      {"\000\037Q\000\003\001", 0x1f55},
#line 732 "./uninorm/composition-table.gperf"
      {"\000\037Q\000\003\000", 0x1f53},
#line 736 "./uninorm/composition-table.gperf"
      {"\000\037Q\000\003B", 0x1f57},
#line 202 "./uninorm/composition-table.gperf"
      {"\000\000o\000\003\033", 0x01a1},
#line 497 "./uninorm/composition-table.gperf"
      {"\000\000p\000\003\001", 0x1e55},
#line 786 "./uninorm/composition-table.gperf"
      {"\000\037&\000\003E", 0x1f96},
#line 139 "./uninorm/composition-table.gperf"
      {"\000\000j\000\003\002", 0x0135},
#line 619 "./uninorm/composition-table.gperf"
      {"\000\000\324\000\003\011", 0x1ed4},
#line 499 "./uninorm/composition-table.gperf"
      {"\000\000p\000\003\007", 0x1e57},
      {""},
#line 538 "./uninorm/composition-table.gperf"
      {"\000\000V\000\003#", 0x1e7e},
#line 324 "./uninorm/composition-table.gperf"
      {"\000\0045\000\003\000", 0x0450},
#line 204 "./uninorm/composition-table.gperf"
      {"\000\000u\000\003\033", 0x01b0},
#line 413 "./uninorm/composition-table.gperf"
      {"\000\000a\000\003%", 0x1e01},
      {""},
#line 602 "./uninorm/composition-table.gperf"
      {"\000\000\352\000\003\011", 0x1ec3},
#line 171 "./uninorm/composition-table.gperf"
      {"\000\000s\000\003'", 0x015f},
#line 931 "./uninorm/composition-table.gperf"
      {"\0000x\0000\231", 0x3079},
      {""},
#line 348 "./uninorm/composition-table.gperf"
      {"\000\0048\000\003\004", 0x04e3},
      {""},
#line 806 "./uninorm/composition-table.gperf"
      {"\000\037j\000\003E", 0x1faa},
#line 814 "./uninorm/composition-table.gperf"
      {"\000\037p\000\003E", 0x1fb2},
#line 622 "./uninorm/composition-table.gperf"
      {"\000\000\364\000\003\003", 0x1ed7},
#line 233 "./uninorm/composition-table.gperf"
      {"\000\001\352\000\003\004", 0x01ec},
      {""},
#line 140 "./uninorm/composition-table.gperf"
      {"\000\000K\000\003'", 0x0136},
#line 347 "./uninorm/composition-table.gperf"
      {"\000\004\030\000\003\004", 0x04e2},
      {""},
#line 505 "./uninorm/composition-table.gperf"
      {"\000\036[\000\003\004", 0x1e5d},
#line 534 "./uninorm/composition-table.gperf"
      {"\000\001j\000\003\010", 0x1e7a},
#line 932 "./uninorm/composition-table.gperf"
      {"\0000x\0000\232", 0x307a},
      {""},
#line 904 "./uninorm/composition-table.gperf"
      {"\000\"\221\000\0038", 0x22e2},
      {""}, {""}, {""},
#line 145 "./uninorm/composition-table.gperf"
      {"\000\000l\000\003'", 0x013c},
      {""},
#line 170 "./uninorm/composition-table.gperf"
      {"\000\000S\000\003'", 0x015e},
      {""},
#line 918 "./uninorm/composition-table.gperf"
      {"\0000[\0000\231", 0x305c},
#line 694 "./uninorm/composition-table.gperf"
      {"\000\003\227\000\003\024", 0x1f29},
#line 325 "./uninorm/composition-table.gperf"
      {"\000\0045\000\003\010", 0x0451},
#line 889 "./uninorm/composition-table.gperf"
      {"\000\"s\000\0038", 0x2275},
      {""},
#line 578 "./uninorm/composition-table.gperf"
      {"\000\000\342\000\003\003", 0x1eab},
#line 679 "./uninorm/composition-table.gperf"
      {"\000\003\225\000\003\023", 0x1f18},
#line 689 "./uninorm/composition-table.gperf"
      {"\000\037 \000\003\001", 0x1f24},
#line 687 "./uninorm/composition-table.gperf"
      {"\000\037 \000\003\000", 0x1f22},
#line 691 "./uninorm/composition-table.gperf"
      {"\000\037 \000\003B", 0x1f26},
      {""},
#line 577 "./uninorm/composition-table.gperf"
      {"\000\000\302\000\003\003", 0x1eaa},
      {""},
#line 952 "./uninorm/composition-table.gperf"
      {"\0000\317\0000\231", 0x30d0},
      {""}, {""},
#line 72 "./uninorm/composition-table.gperf"
      {"\000\000c\000\003'", 0x00e7},
#line 970 "./uninorm/composition-table.gperf"
      {"\001\020\245\001\020\272", 0x110ab},
#line 201 "./uninorm/composition-table.gperf"
      {"\000\000O\000\003\033", 0x01a0},
#line 780 "./uninorm/composition-table.gperf"
      {"\000\037 \000\003E", 0x1f90},
      {""},
#line 406 "./uninorm/composition-table.gperf"
      {"\000\033\021\000\0335", 0x1b12},
      {""}, {""},
#line 404 "./uninorm/composition-table.gperf"
      {"\000\033\013\000\0335", 0x1b0c},
#line 428 "./uninorm/composition-table.gperf"
      {"\000\000D\000\003'", 0x1e10},
      {""},
#line 953 "./uninorm/composition-table.gperf"
      {"\0000\317\0000\232", 0x30d1},
#line 411 "./uninorm/composition-table.gperf"
      {"\000\033B\000\0335", 0x1b43},
#line 454 "./uninorm/composition-table.gperf"
      {"\000\000H\000\003.", 0x1e2a},
      {""},
#line 340 "./uninorm/composition-table.gperf"
      {"\000\0045\000\003\006", 0x04d7},
      {""}, {""}, {""},
#line 412 "./uninorm/composition-table.gperf"
      {"\000\000A\000\003%", 0x1e00},
      {""},
#line 971 "./uninorm/composition-table.gperf"
      {"\001\0211\001\021'", 0x1112e},
#line 383 "./uninorm/composition-table.gperf"
      {"\000\013\222\000\013\327", 0x0b94},
      {""},
#line 203 "./uninorm/composition-table.gperf"
      {"\000\000U\000\003\033", 0x01af},
#line 620 "./uninorm/composition-table.gperf"
      {"\000\000\364\000\003\011", 0x1ed5},
      {""}, {""}, {""}, {""},
#line 379 "./uninorm/composition-table.gperf"
      {"\000\011\307\000\011\327", 0x09cc},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 892 "./uninorm/composition-table.gperf"
      {"\000\"z\000\0038", 0x2280},
      {""},
#line 388 "./uninorm/composition-table.gperf"
      {"\000\014\277\000\014\325", 0x0cc0},
      {""}, {""}, {""}, {""}, {""},
#line 455 "./uninorm/composition-table.gperf"
      {"\000\000h\000\003.", 0x1e2b},
#line 576 "./uninorm/composition-table.gperf"
      {"\000\000\342\000\003\011", 0x1ea9},
      {""}, {""}, {""}, {""},
#line 680 "./uninorm/composition-table.gperf"
      {"\000\003\225\000\003\024", 0x1f19},
#line 575 "./uninorm/composition-table.gperf"
      {"\000\000\302\000\003\011", 0x1ea8},
#line 223 "./uninorm/composition-table.gperf"
      {"\000\002&\000\003\004", 0x01e0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 396 "./uninorm/composition-table.gperf"
      {"\000\015\331\000\015\312", 0x0dda},
#line 876 "./uninorm/composition-table.gperf"
      {"\000\"%\000\0038", 0x2226},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 367 "./uninorm/composition-table.gperf"
      {"\000\006'\000\006S", 0x0622},
#line 141 "./uninorm/composition-table.gperf"
      {"\000\000k\000\003'", 0x0137},
      {""},
#line 380 "./uninorm/composition-table.gperf"
      {"\000\013G\000\013V", 0x0b48},
#line 237 "./uninorm/composition-table.gperf"
      {"\000\000j\000\003\014", 0x01f0},
#line 385 "./uninorm/composition-table.gperf"
      {"\000\013\307\000\013\276", 0x0bcb},
      {""},
#line 875 "./uninorm/composition-table.gperf"
      {"\000\"#\000\0038", 0x2224},
      {""},
#line 973 "./uninorm/composition-table.gperf"
      {"\001\023G\001\023>", 0x1134b},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 909 "./uninorm/composition-table.gperf"
      {"\000\"\265\000\0038", 0x22ed},
#line 369 "./uninorm/composition-table.gperf"
      {"\000\006H\000\006T", 0x0624},
      {""}, {""}, {""},
#line 398 "./uninorm/composition-table.gperf"
      {"\000\015\334\000\015\312", 0x0ddd},
      {""}, {""},
#line 390 "./uninorm/composition-table.gperf"
      {"\000\014\306\000\014\326", 0x0cc8},
#line 391 "./uninorm/composition-table.gperf"
      {"\000\014\306\000\014\302", 0x0cca},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 389 "./uninorm/composition-table.gperf"
      {"\000\014\306\000\014\325", 0x0cc7},
      {""}, {""}, {""},
#line 913 "./uninorm/composition-table.gperf"
      {"\0000Q\0000\231", 0x3052},
#line 372 "./uninorm/composition-table.gperf"
      {"\000\006\325\000\006T", 0x06c0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
#line 900 "./uninorm/composition-table.gperf"
      {"\000\"\251\000\0038", 0x22ae},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 974 "./uninorm/composition-table.gperf"
      {"\001\023G\001\023W", 0x1134c},
#line 976 "./uninorm/composition-table.gperf"
      {"\001\024\271\001\024\260", 0x114bc},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 409 "./uninorm/composition-table.gperf"
      {"\000\033>\000\0335", 0x1b40},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 392 "./uninorm/composition-table.gperf"
      {"\000\014\312\000\014\325", 0x0ccb},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 361 "./uninorm/composition-table.gperf"
      {"\000\004#\000\003\013", 0x04f2},
      {""}, {""}, {""}, {""}, {""},
#line 374 "./uninorm/composition-table.gperf"
      {"\000\006\322\000\006T", 0x06d3},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
#line 979 "./uninorm/composition-table.gperf"
      {"\001\025\271\001\025\257", 0x115bb},
      {""}, {""}, {""},
#line 370 "./uninorm/composition-table.gperf"
      {"\000\006'\000\006U", 0x0625},
      {""}, {""}, {""}, {""}, {""},
#line 899 "./uninorm/composition-table.gperf"
      {"\000\"\250\000\0038", 0x22ad},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 871 "./uninorm/composition-table.gperf"
      {"\000!\322\000\0038", 0x21cf},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 384 "./uninorm/composition-table.gperf"
      {"\000\013\306\000\013\276", 0x0bca},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
#line 373 "./uninorm/composition-table.gperf"
      {"\000\006\301\000\006T", 0x06c2},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 400 "./uninorm/composition-table.gperf"
      {"\000\020%\000\020.", 0x1026},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 386 "./uninorm/composition-table.gperf"
      {"\000\013\306\000\013\327", 0x0bcc},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
#line 870 "./uninorm/composition-table.gperf"
      {"\000!\324\000\0038", 0x21ce}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = gl_uninorm_compose_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].codes;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
