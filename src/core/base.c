#include "base.h"

void base_reset(base_state_t *b)
{
    b->radix = 10;
    b->bits = 32;
    b->carry = false;
}

void base_validate(base_state_t *b)
{
    if (b->radix != 10 && b->radix != 16 && b->radix != 8 && b->radix != 2)
        b->radix = 10;
    if (b->bits < 1 || b->bits > BASE_MAX_BITS)
        b->bits = 32;
}

base_word_t base_mask(const base_state_t *b)
{
    return ((base_word_t)1 << b->bits) - 1;
}

/* ---------- words and reals ---------- */

#define CHUNK_BITS 22       /* reals are split into 22-bit halves, which the
                             * OS's 24-bit conversions handle exactly */

/* A whole number from 0 to 2^44 - 1 as a word, or why it isn't one. */
static base_fit_t real_to_word(const real_t *v, base_word_t *out)
{
    real_t zero = os_Int24ToReal(0);
    real_t f = os_RealFrac(v);
    if (os_RealCompare(&f, &zero) != 0)
        return BW_FRACTION;
    if (os_RealCompare(v, &zero) < 0)
        return BW_NEGATIVE;
    real_t limit = os_StrToReal("17592186044416", NULL);   /* 2^44 */
    if (os_RealCompare(v, &limit) >= 0)
        return BW_TOO_BIG;
    real_t k = os_Int24ToReal(1 << CHUNK_BITS);
    real_t q = os_RealDiv(v, &k);
    real_t hi = os_RealInt(&q);
    real_t hik = os_RealMul(&hi, &k);
    real_t lo = os_RealSub(v, &hik);
    *out = ((base_word_t)os_RealToInt24(&hi) << CHUNK_BITS) |
           (base_word_t)os_RealToInt24(&lo);
    return BW_OK;
}

bool base_u32(const real_t *v, uint32_t *out)
{
    base_word_t w;
    if (real_to_word(v, &w) != BW_OK || w > 0xFFFFFFFFUL)
        return false;
    *out = (uint32_t)w;
    return true;
}

base_fit_t base_fit(const base_state_t *b, const real_t *v, base_word_t *out)
{
    base_word_t w;
    base_fit_t f = real_to_word(v, &w);
    if (f != BW_OK)
        return f;
    if (w > base_mask(b))
        return BW_TOO_BIG;
    *out = w;
    return BW_OK;
}

bool base_word(const base_state_t *b, const real_t *v, base_word_t *out)
{
    return base_fit(b, v, out) == BW_OK;
}

real_t base_real(base_word_t w)
{
    real_t hi = os_Int24ToReal((int)(w >> CHUNK_BITS));
    real_t k = os_Int24ToReal(1 << CHUNK_BITS);
    real_t lo = os_Int24ToReal((int)(w & ((1 << CHUNK_BITS) - 1)));
    real_t t = os_RealMul(&hi, &k);
    return os_RealAdd(&t, &lo);
}

void base_format(char *out, const base_state_t *b, base_word_t w)
{
    char tmp[48];
    int n = 0;
    do {
        unsigned d = (unsigned)(w % b->radix);
        tmp[n++] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
        w /= b->radix;
    } while (w);
    for (int i = 0; i < n; i++)
        out[i] = tmp[n - 1 - i];
    out[n] = '\0';
}

int base_digit_value(const base_state_t *b, char ch)
{
    int v = (ch >= '0' && ch <= '9') ? ch - '0' :
            (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10 : -1;
    return (v >= 0 && v < b->radix) ? v : -1;
}

/* ---------- operations ---------- */

bool base_is_binary(base_op_t op)
{
    switch (op) {
        case BOP_AND: case BOP_OR: case BOP_XOR:
        case BOP_SLN: case BOP_SRN: case BOP_RLN: case BOP_RRN:
        case BOP_RLCN: case BOP_RRCN:
        case BOP_CB: case BOP_SB: case BOP_GETB:
        case BOP_ADD: case BOP_SUB: case BOP_MUL: case BOP_DIV: case BOP_DIV2:
            return true;
        default:
            return false;
    }
}

/* One-bit rotates. `through` rotates through the carry flag (RLC, RRC). */
static base_word_t rotate_left(base_state_t *b, base_word_t w, bool through)
{
    base_word_t top = (w >> (b->bits - 1)) & 1;
    base_word_t in = through ? (base_word_t)b->carry : top;
    b->carry = top != 0;
    return ((w << 1) | in) & base_mask(b);
}

static base_word_t rotate_right(base_state_t *b, base_word_t w, bool through)
{
    base_word_t low = w & 1;
    base_word_t in = through ? (base_word_t)b->carry : low;
    b->carry = low != 0;
    return (w >> 1) | (in << (b->bits - 1));
}

bool base_apply(base_state_t *b, base_op_t op, base_word_t y, base_word_t x,
                base_word_t *result, base_word_t *result2)
{
    const base_word_t m = base_mask(b);
    const unsigned n = b->bits;
    base_word_t r = 0;

    switch (op) {
        case BOP_AND: r = y & x; break;
        case BOP_OR:  r = y | x; break;
        case BOP_XOR: r = y ^ x; break;
        case BOP_NOT: r = ~x & m; break;
        case BOP_NEG: r = (0 - x) & m; break;              /* two's complement */

        case BOP_SL:
            b->carry = ((x >> (n - 1)) & 1) != 0;
            r = (x << 1) & m;
            break;
        case BOP_SR:
            b->carry = (x & 1) != 0;
            r = x >> 1;
            break;
        case BOP_ASR:                   /* keeps the sign bit */
            b->carry = (x & 1) != 0;
            r = (x >> 1) | (x & ((base_word_t)1 << (n - 1)));
            break;
        case BOP_SLN:                   /* Y shifted left X bits */
            if (x > n) return false;
            if (x > 0) {
                b->carry = ((y >> (n - x)) & 1) != 0;
                r = (x == n) ? 0 : (y << x) & m;
            } else {
                r = y;
            }
            break;
        case BOP_SRN:
            if (x > n) return false;
            if (x > 0) {
                b->carry = ((y >> (x - 1)) & 1) != 0;
                r = (x == n) ? 0 : y >> x;
            } else {
                r = y;
            }
            break;

        case BOP_RL:  r = rotate_left(b, x, false); break;
        case BOP_RR:  r = rotate_right(b, x, false); break;
        case BOP_RLC: r = rotate_left(b, x, true); break;
        case BOP_RRC: r = rotate_right(b, x, true); break;
        case BOP_RLN: case BOP_RRN: case BOP_RLCN: case BOP_RRCN: {
            if (x > n) return false;    /* Y rotated X times */
            bool left = (op == BOP_RLN || op == BOP_RLCN);
            bool through = (op == BOP_RLCN || op == BOP_RRCN);
            r = y;
            for (base_word_t i = 0; i < x; i++)
                r = left ? rotate_left(b, r, through) : rotate_right(b, r, through);
            break;
        }

        case BOP_CB: case BOP_SB: case BOP_GETB:    /* bit X of Y */
            if (x >= n) return false;
            if (op == BOP_CB)      r = y & ~((base_word_t)1 << x);
            else if (op == BOP_SB) r = y | ((base_word_t)1 << x);
            else                   r = (y >> x) & 1;
            break;
        case BOP_REVB:
            for (unsigned i = 0; i < n; i++)
                if (x & ((base_word_t)1 << i))
                    r |= (base_word_t)1 << (n - 1 - i);
            break;
        case BOP_CNTB:
            for (; x; x &= x - 1)
                r++;
            break;

        case BOP_ADD:                   /* words are at most 44 bits, so a 64-bit */
            r = y + x;                  /* sum can't overflow */
            b->carry = r > m;
            r &= m;
            break;
        case BOP_SUB:
            b->carry = y < x;           /* borrow */
            r = (y - x) & m;
            break;
        case BOP_MUL:                   /* overflow if the product needs more than */
            b->carry = x != 0 && y > m / x;     /* the word size */
            r = (y * x) & m;            /* the low bits (mod 2^64, then masked) */
            break;
        case BOP_DIV:
        case BOP_DIV2:
            if (x == 0) return false;
            r = y / x;
            if (result2)
                *result2 = y % x;
            break;
    }
    *result = r;
    return true;
}
