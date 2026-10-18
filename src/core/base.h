#ifndef RPNCE_CORE_BASE_H
#define RPNCE_CORE_BASE_H

/*
 * BASE mode: integers shown and entered in HEX, OCT or BIN, with logic,
 * shift, rotate and bit operations on unsigned words of 8, 16, 24 or 32
 * bits (RPN83P's sizes: the calculator's 14-digit numbers hold any 32-bit
 * value exactly, but not larger words).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ti/real.h>

typedef struct {
    uint8_t radix;      /* 10 (DEC: normal numbers), 16, 8 or 2 */
    uint8_t bits;       /* word size: 8, 16, 24 or 32 */
    bool carry;         /* carry flag, set by shifts, rotates, BAS+ and BAS- */
} base_state_t;

void base_reset(base_state_t *b);           /* DEC, 32 bits, carry clear */
void base_validate(base_state_t *b);        /* fixes bad saved values */

uint32_t base_mask(const base_state_t *b);

/* A whole number from 0 to 2^32 - 1 (also used by PRIM). */
bool base_u32(const real_t *v, uint32_t *out);

/* X as a word: false if X isn't a whole number from 0 to 2^bits - 1. */
bool base_word(const base_state_t *b, const real_t *v, uint32_t *out);
real_t base_real(uint32_t w);

/* Digits of a word in the current radix ("1F", "777", "1011"). */
void base_format(char *out, const base_state_t *b, uint32_t w);

/* The value of a digit character in the radix, or -1. */
int base_digit_value(const base_state_t *b, char ch);

/* Operations. Unary ones use x; the rest y op x. Results are masked to the
 * word size; shifts and rotates set the carry to the last bit moved out.
 * Return false for a bad argument (bit numbers, shift counts, divide by 0). */
typedef enum {
    BOP_AND, BOP_OR, BOP_XOR, BOP_NOT, BOP_NEG,
    BOP_SL, BOP_SR, BOP_ASR, BOP_SLN, BOP_SRN,
    BOP_RL, BOP_RR, BOP_RLC, BOP_RRC,
    BOP_RLN, BOP_RRN, BOP_RLCN, BOP_RRCN,
    BOP_CB, BOP_SB, BOP_GETB, BOP_REVB, BOP_CNTB,
    BOP_ADD, BOP_SUB, BOP_MUL, BOP_DIV, BOP_DIV2,
} base_op_t;

bool base_is_binary(base_op_t op);

/* For BOP_DIV2, *result is the quotient and *result2 the remainder. */
bool base_apply(base_state_t *b, base_op_t op, uint32_t y, uint32_t x,
                uint32_t *result, uint32_t *result2);

#endif
