#define ARRAY_GLOBAL
#define ARRAY_NAME string
#define ARRAY_TYPE char
#include "generic/array.g"

#define NAME_CAT_EXPAND(a, b) a ## b
#define NAME_CAT(a, b) NAME_CAT_EXPAND(a, b)
#define NAME(suffix) NAME_CAT(ARRAY_NAME, suffix)
int NAME(_append)(struct ARRAY_NAME *restrict string, const ARRAY_TYPE *data, size_t count);
#undef NAME
#undef NAME_CAT
#undef NAME_CAT_EXPAND
