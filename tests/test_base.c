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
static void t(const char *name, base_op_t op, uint32_t y, uint32_t x, uint32_t want, int want_carry)
{
    uint32_t r = 0, r2 = 0;
    bool ok = base_apply(&B, op, y, x, &r, &r2);
    int good = ok && r == want && (want_carry < 0 || B.carry == want_carry);
    if (!good) fails++;
    printf("%s %-30s %08X carry %d (want %08X%s)\n", good ? "ok  " : "FAIL", name,
           (unsigned)r, B.carry, (unsigned)want, want_carry < 0 ? "" : want_carry ? " carry 1" : " carry 0");
}
static void bad(const char *name, base_op_t op, uint32_t y, uint32_t x)
{
    uint32_t r;
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
    uint32_t q, rm;
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

    uint32_t w;
    real_t big = mk(4294967295.0);
    bool ok = base_u32(&big, &w) && w == 0xFFFFFFFF;
    printf("%s real 4294967295 -> word\n", ok ? "ok  " : "FAIL"); if (!ok) fails++;
    real_t back = base_real(0xFFFFFFFF);
    printf("%s word -> real %.0f\n", back.v == 4294967295.0 ? "ok  " : "FAIL", back.v); if (back.v != 4294967295.0) fails++;

    printf("%d failures\n", fails);
    return fails != 0;
}
