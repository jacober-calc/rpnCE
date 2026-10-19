/* In-memory appvars supplied by test_storage.c. */
#ifndef MOCK_STORAGE_FILEIOC_H
#define MOCK_STORAGE_FILEIOC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
uint8_t ti_Open(const char *name, const char *mode);
size_t ti_Read(void *data, size_t size, size_t count, uint8_t handle);
size_t ti_Write(const void *data, size_t size, size_t count, uint8_t handle);
int ti_Close(uint8_t handle);
bool ti_SetArchiveStatus(bool archived, uint8_t handle);
int ti_Delete(const char *name);
#endif
