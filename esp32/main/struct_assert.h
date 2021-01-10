#ifndef __STRUCT_ASSERT_H__
#define __STRUCT_ASSERT_H__

#include "esp_assert.h"

#define ASSERT_STRUCT_SIZE(STRUCT, SIZE) _Static_assert( sizeof(STRUCT)==SIZE, "size of " #STRUCT " is not " #SIZE )

#endif