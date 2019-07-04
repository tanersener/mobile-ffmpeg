#ifndef OPTIMIZED_SAD_FUNC_T_H_
#define OPTIMIZED_SAD_FUNC_T_H_

#include "kvazaar.h"

/**
 * \param data1: Picture block pointer
 * \param data2: Reference block pointer
 * \param height: Scan block height
 * \param stride1: Picture block stride
 * \param stride2: Reference block stride
 */
typedef uint32_t (*optimized_sad_func_ptr_t)(const kvz_pixel * const,
                                             const kvz_pixel * const,
                                             const int32_t,
                                             const uint32_t,
                                             const uint32_t);

#endif
