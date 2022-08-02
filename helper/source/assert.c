/**
 *     Copyright (c) 2021, Micha Hoiting.
 *
 * \file  rv-link/details/assert.c
 * \brief Implementation of the rvl_assert.
  */

/* other project header file includes */
#include <stdint.h>
#include "debug.h"

void rvl_assert_handler(const char *cond, const char *func, uint32_t line)
{
    RVL_DEBUG_LOG("(%s) assertion failed at function:%s, line number:%d \n", cond, func, line);

    for(;;) {
      /* halt */
    }
}
