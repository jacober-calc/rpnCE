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
    if (b->bits != 8 && b->bits != 16 && b->bits != 24 && b->bits != 32)
        b->bits = 32;
}

uint32_t base_mask(const base_state_t *b)
{
    return (b->bits >= 32) ? 0xFFFFFFFFUL : ((1UL << b->bits) - 1);
}

/* ---------- words and reals ---------- */

/* A whole number from 0 to 2^32 - 1, split into 16-bit halves so the OS's
 * 24-bit conversions can handle it. */
bool base_u32(const real_t *v, uint32_t *out)
{
    real_t zero = os_Int24ToReal(0);
    real_t max = os_StrToReal("4294967295", NULL);
    real_t f = os_RealFrac(v);
    if (os_RealCompare(&f, &zero) != 0 || os_RealCompare(v, &zero) < 0 ||
        os_RealCompare(v, &max) > 0)
        return false;
    real_t k = os_Int24ToReal(65536);
    real_t q = os_RealDiv(v, &k);
    real_t hi = os_RealInt(&q);
    real_t hik = os_RealMul(&hi, &k);
    real_t lo = os_RealSub(v, &hik);
    *out = ((uint32_t)os_RealToInt24(&hi) << 16) | (uint32_t)os_RealToInt24(&lo);
    return true;
}

bool base_word(const base_state_t *b, const real_t *v, uint32_t *out)
{
    return base_u32(v, out) && *out <= base_mask(b);
}

real_t base_real(uint32_t w)
{
    real_t hi = os_Int24ToReal((int)(w >> 16));
    real_t k = os_Int24ToReal(65536);
    real_t lo = os_Int24ToReal((int)(w & 0xFFFF));
    real_t t = os_RealMul(&hi, &k);
    return os_RealAdd(&t, &lo);
}

void base_format(char *out, const base_state_t *b, uint32_t w)
{
    char tmp[34];
    int n = 0;
    do {
        uint32_t d = w % b->radix;
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
static uint32_t rotate_left(base_state_t *b, uint32_t w, bool through)
{
    uint32_t top = (w >> (b->bits - 1)) & 1;
    uint32_t in = through ? (uint32_t)b->carry : top;
    b->carry = top != 0;
    return ((w << 1) | in) & base_mask(b);
}

static uint32_t rotate_right(base_state_t *b, uint32_t w, bool through)
{
    uint32_t low = w & 1;
    uint32_t in = through ? (uint32_t)b->carry : low;
    b->carry = low != 0;
    return (w >> 1) | (in << (b->bits - 1));
}

bool base_apply(base_state_t *b, base_op_t op, uint32_t y, uint32_t x,
                uint32_t *result, uint32_t *result2)
{
    const uint32_t m = base_mask(b);
    const unsigned n = b->bits;
    uint64_t wide;
    uint32_t r = 0;

    switch (op) {
        case BOP_AND: r = y & x; break;
        case BOP_OR:  r = y | x; break;
        case BOP_XOR: r = y ^ x; break;
        case BOP_NOT: r = ~x & m; break;
        case BOP_NEG: r = (uint32_t)(0u - x) & m; break;   /* two's complement */

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
            r = (x >> 1) | (x & (1UL << (n - 1)));
            break;
        case BOP_SLN:                   /* Y shifted left X bits */
            if (x > n) return false;
            if (x > 0) {
                b->carry = ((y >> (n - x)) & 1) != 0;
                r = (x == n) ? 0 : (uint32_t)(((uint64_t)y << x) & m);
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
            for (uint32_t i = 0; i < x; i++)
                r = left ? rotate_left(b, r, through) : rotate_right(b, r, through);
            break;
        }

        case BOP_CB: case BOP_SB: case BOP_GETB:    /* bit X of Y */
            if (x >= n) return false;
            if (op == BOP_CB)      r = y & ~(1UL << x);
            else if (op == BOP_SB) r = y | (1UL << x);
            else                   r = (y >> x) & 1;
            break;
        case BOP_REVB:
            for (unsigned i = 0; i < n; i++)
                if (x & (1UL << i))
                    r |= 1UL << (n - 1 - i);
            break;
        case BOP_CNTB:
            for (; x; x &= x - 1)
                r++;
            break;

        case BOP_ADD:
            wide = (uint64_t)y + x;
            b->carry = wide > m;
            r = (uint32_t)(wide & m);
            break;
        case BOP_SUB:
            b->carry = y < x;           /* borrow */
            r = (y - x) & m;
            break;
        case BOP_MUL:
            wide = (uint64_t)y * x;
            b->carry = wide > m;
            r = (uint32_t)(wide & m);
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
