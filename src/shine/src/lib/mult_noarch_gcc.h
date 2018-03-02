#include <stdint.h>

#ifndef mul
#define mul(a,b)   (int32_t)  ( ( ((int64_t) a) * ((int64_t) b) ) >>32 )
#endif

#ifndef muls
#define muls(a,b)  (int32_t)  ( ( ((int64_t) a) * ((int64_t) b) ) >>31 )
#endif

#ifndef mulr
#define mulr(a,b)  (int32_t)  ( ( ( ((int64_t) a) * ((int64_t) b)) + 0x80000000LL ) >>32 )
#endif

#ifndef mulsr
#define mulsr(a,b) (int32_t)  ( ( ( ((int64_t) a) * ((int64_t) b)) + 0x40000000LL ) >>31 )
#endif

#ifndef mul0
#define mul0(hi,lo,a,b)     ((hi)  = mul((a), (b)))
#define muladd(hi,lo,a,b)   ((hi) += mul((a), (b)))
#define mulsub(hi,lo,a,b)   ((hi) -= mul((a), (b)))
#define mulz(hi,lo)
#endif

#ifndef cmuls
#define cmuls(dre, dim, are, aim, bre, bim) \
do { \
	int32_t tre; \
	(tre) = (int32_t) (((int64_t) (are) * (int64_t) (bre) - (int64_t) (aim) * (int64_t) (bim)) >> 31); \
	(dim) = (int32_t) (((int64_t) (are) * (int64_t) (bim) + (int64_t) (aim) * (int64_t) (bre)) >> 31); \
	(dre) = tre; \
} while (0)
#endif
