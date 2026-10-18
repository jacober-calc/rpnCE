#ifndef MOCK_TI_ERROR_H
#define MOCK_TI_ERROR_H
#include <setjmp.h>
#define OS_E_EDIT (1<<7)
#define OS_E_DIVBY0 (2+OS_E_EDIT)
#define OS_E_DOMAIN (4+OS_E_EDIT)
#define OS_E_SIGNCHANGE (24+OS_E_EDIT)
#define OS_E_ITERATIONS (25+OS_E_EDIT)
#define OS_E_STAT (21)
#define OS_E_MASK 0x7F
#define OS_E_OVERFLOW (1+OS_E_EDIT)
#define OS_E_NONREAL (7+OS_E_EDIT)
#define os_PushErrorHandler() setjmp(mock_jmp)
#define os_PopErrorHandler() ((void)0)
extern jmp_buf mock_jmp;
static inline void os_ThrowError(int e) { longjmp(mock_jmp, e); }
#endif
