/* Tests for BASE operations, on a PC:
 *   gcc -Itests/mock -Isrc/core -o test_base tests/test_base.c src/core/base.c -lm
 */
#include <stdio.h>
#include <string.h>
#include <ti/real.h>
#include "base.h"

real_t mock_fin[7];
real_t os_RealDiv(const real_t *a, const real_t *b) { return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a) { return mk(log(a->v)); }

static int fails = 0;
static base_state_t B;
static void t(const char *name, base_op_t op, base_word_t y, base_word_t x, base_word_t want, int want_carry)
{
    base_word_t r = 0, r2 = 0;
    bool ok = base_apply(&B, op, y, x, &r, &r2);
    int good = ok && r == want && (want_carry < 0 || B.carry == want_carry);
    if (!good) fails++;
    printf("%s %-30s %011llX carry %d (want %011llX%s)\n", good ? "ok  " : "FAIL", name,
           (unsigned long long)r, B.carry, (unsigned long long)want,
           want_carry < 0 ? "" : want_carry ? " carry 1" : " carry 0");
}
static void bad(const char *name, base_op_t op, base_word_t y, base_word_t x)
{
    base_word_t r;
    bool ok = base_apply(&B, op, y, x, &r, NULL);
    if (ok) fails++;
    printf("%s %s is refused\n", ok ? "FAIL" : "ok  ", name);
}

int main(void)
{
    base_reset(&B);
    B.bits = 8;
    t("AND", BOP_AND, 0xF0, 0x3C, 0x30, -1);
    t("OR", BOP_OR, 0xF0, 0x0F, 0xFF, -1);
    t("XOR", BOP_XOR, 0xFF, 0x0F, 0xF0, -1);
    t("NOT 8-bit", BOP_NOT, 0, 0x0F, 0xF0, -1);
    t("NEG 8-bit", BOP_NEG, 0, 1, 0xFF, -1);
    t("SL carry out", BOP_SL, 0, 0x81, 0x02, 1);
    t("SR carry out", BOP_SR, 0, 0x03, 0x01, 1);
    t("ASR keeps sign", BOP_ASR, 0, 0x80, 0xC0, 0);
    t("SLn 3", BOP_SLN, 0x31, 3, 0x88, 1);
    t("SRn 4", BOP_SRN, 0xF8, 4, 0x0F, 1);
    t("SLn 8 clears", BOP_SLN, 0xFF, 8, 0x00, 1);
    t("RL", BOP_RL, 0, 0x81, 0x03, 1);
    t("RR", BOP_RR, 0, 0x01, 0x80, 1);
    B.carry = 0;
    t("RLC with carry 0", BOP_RLC, 0, 0x80, 0x00, 1);
    t("RLC with carry 1", BOP_RLC, 0, 0x00, 0x01, 0);
    t("RRn 4", BOP_RRN, 0x12, 4, 0x21, -1);
    t("CB bit 7", BOP_CB, 0xFF, 7, 0x7F, -1);
    t("SB bit 0", BOP_SB, 0x00, 0, 0x01, -1);
    t("B? bit 4", BOP_GETB, 0x10, 4, 1, -1);
    t("REVB 8-bit", BOP_REVB, 0, 0x01, 0x80, -1);
    t("CNTB", BOP_CNTB, 0, 0xB7, 6, -1);
    t("BAS+ overflow", BOP_ADD, 0xFF, 0x02, 0x01, 1);
    t("BAS- borrow", BOP_SUB, 0x01, 0x02, 0xFF, 1);
    t("BAS* overflow", BOP_MUL, 0x10, 0x10, 0x00, 1);
    t("BAS/", BOP_DIV, 100, 7, 14, -1);
    bad("BAS/ by zero", BOP_DIV, 5, 0);
    bad("bit 8 of an 8-bit word", BOP_GETB, 1, 8);
    bad("shift by 9 in 8 bits", BOP_SLN, 1, 9);
    B.bits = 32;
    t("NOT 32-bit", BOP_NOT, 0, 0, 0xFFFFFFFF, -1);
    t("BAS+ 32-bit carry", BOP_ADD, 0xFFFFFFFF, 1, 0, 1);
    t("RL 32-bit", BOP_RL, 0, 0x80000000, 1, 1);
    t("REVB 32-bit", BOP_REVB, 0, 1, 0x80000000, -1);
    base_word_t q, rm;
    base_apply(&B, BOP_DIV2, 100, 7, &q, &rm);
    printf("%s BDIV 100/7 = %u rem %u\n", q == 14 && rm == 2 ? "ok  " : "FAIL", (unsigned)q, (unsigned)rm);
    if (!(q == 14 && rm == 2)) fails++;

    char buf[40];
    B.radix = 16; base_format(buf, &B, 0xDEADBEEF);
    printf("%s HEX %s\n", strcmp(buf, "DEADBEEF") == 0 ? "ok  " : "FAIL", buf); if (strcmp(buf, "DEADBEEF")) fails++;
    B.radix = 2; base_format(buf, &B, 10);
    printf("%s BIN %s\n", strcmp(buf, "1010") == 0 ? "ok  " : "FAIL", buf); if (strcmp(buf, "1010")) fails++;
    B.radix = 8; base_format(buf, &B, 511);
    printf("%s OCT %s\n", strcmp(buf, "777") == 0 ? "ok  " : "FAIL", buf); if (strcmp(buf, "777")) fails++;

    uint32_t w32;
    real_t big = mk(4294967295.0);
    bool ok = base_u32(&big, &w32) && w32 == 0xFFFFFFFF;
    printf("%s real 4294967295 -> word\n", ok ? "ok  " : "FAIL"); if (!ok) fails++;
    real_t back = base_real(0xFFFFFFFF);
    printf("%s word -> real %.0f\n", back.v == 4294967295.0 ? "ok  " : "FAIL", back.v); if (back.v != 4294967295.0) fails++;

    /* 44-bit words */
    B.bits = 44; B.radix = 16; B.carry = 0;
    base_word_t w;
    real_t v = mk(5133136320.0);
    ok = base_fit(&B, &v, &w) == BW_OK && w == 0x131F571C0ULL;
    base_format(buf, &B, w);
    printf("%s 5133136320 in HEX (44 bits) = %s\n", ok && strcmp(buf, "131F571C0") == 0 ? "ok  " : "FAIL", buf);
    if (!(ok && strcmp(buf, "131F571C0") == 0)) fails++;
    B.bits = 32;
    ok = base_fit(&B, &v, &w) == BW_TOO_BIG;
    printf("%s 5133136320 exceeds a 32-bit word\n", ok ? "ok  " : "FAIL"); if (!ok) fails++;
    real_t neg = mk(-5), frac = mk(2.5), top = mk(17592186044415.0), over = mk(17592186044416.0);
    B.bits = 44;
    ok = base_fit(&B, &neg, &w) == BW_NEGATIVE && base_fit(&B, &frac, &w) == BW_FRACTION;
    printf("%s negative and fractional values are classified\n", ok ? "ok  " : "FAIL"); if (!ok) fails++;
    ok = base_fit(&B, &top, &w) == BW_OK && w == 0xFFFFFFFFFFFULL && base_fit(&B, &over, &w) == BW_TOO_BIG;
    printf("%s 2^44-1 fits, 2^44 doesn't\n", ok ? "ok  " : "FAIL"); if (!ok) fails++;
    back = base_real(0xFFFFFFFFFFFULL);
    printf("%s 44-bit word -> real %.0f\n", back.v == 17592186044415.0 ? "ok  " : "FAIL", back.v); if (back.v != 17592186044415.0) fails++;
    t("NOT 44-bit", BOP_NOT, 0, 0, 0xFFFFFFFFFFFULL, -1);
    t("BAS+ 44-bit carry", BOP_ADD, 0xFFFFFFFFFFFULL, 1, 0, 1);
    t("BAS* 44-bit, no overflow", BOP_MUL, 0x100000, 0x100000, 0x10000000000ULL, 0);
    t("BAS* 44-bit overflow", BOP_MUL, 0x1000000, 0x100000, 0, 1);
    t("RL 44-bit", BOP_RL, 0, 0x80000000000ULL, 1, 1);
    t("REVB 44-bit", BOP_REVB, 0, 1, 0x80000000000ULL, -1);
    t("SLn 40 in 44 bits", BOP_SLN, 1, 40, 0x10000000000ULL, 0);
    B.bits = 12;
    t("NOT in a 12-bit word", BOP_NOT, 0, 0, 0xFFF, -1);
    t("RR in a 12-bit word", BOP_RR, 0, 1, 0x800, 1);

    printf("%d failures\n", fails);
    return fails != 0;
}
