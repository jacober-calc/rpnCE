#ifndef MOCK_UTIL_H
#define MOCK_UTIL_H
#include <stdint.h>
#include <stdlib.h>
#define random mock_random
#define srandom mock_srandom
static inline uint32_t mock_random(void) { return (uint32_t)rand(); }
static inline void mock_srandom(uint32_t s) { srand(s); }
#endif
