/* PC stand-in for fileioc: "opens" RPNCEres from data/rpnCEres.bin. */
#ifndef MOCK_FILEIOC_H
#define MOCK_FILEIOC_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
extern uint8_t mock_res[65536];
extern size_t mock_res_size;
static inline uint8_t ti_Open(const char *n, const char *m)
{ (void)m; return (strcmp(n, "RPNCEres") == 0 && mock_res_size) ? 1 : 0; }
static inline size_t ti_GetSize(uint8_t h) { (void)h; return mock_res_size; }
static inline void *ti_GetDataPtr(uint8_t h) { (void)h; return mock_res; }
static inline int ti_Close(uint8_t h) { (void)h; return 1; }
#endif
