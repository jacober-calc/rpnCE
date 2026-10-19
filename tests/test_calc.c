/* Tests for the calculator core: stack, registers, (i), units on the stack.
 * Runs the real calc.c on a PC with stand-ins for the OS number routines:
 *   sh tests/run_tests.sh
 */
#include <stdio.h>
#include <string.h>
#include <ti/real.h>
#include <ti/error.h>
#include "calc.h"
#include "dateobj.h"

#include "mock_calc_runtime.h"

static calc_t C;
static int fails = 0;

/* types a number and runs commands, like keystrokes */
static void num(double v)
{
    char buf[32];
    sprintf(buf, "%.12g", v);
    for (char *p = buf; *p; p++) {
        if (*p == '-') continue;
        if (*p == '.') calc_execute(&C, CMD_DOT, 0);
        else if (*p == 'e') calc_execute(&C, CMD_EE, 0);
        else calc_execute(&C, CMD_DIGIT, (uint8_t)(*p - '0'));
    }
    if (v < 0) calc_execute(&C, CMD_CHS, 0);
}
static void cmd(calc_cmd_t k, uint8_t arg) { calc_execute(&C, k, arg); }

static void expect(const char *name, double got, double want)
{
    int ok = fabs(got - want) <= 1e-9 * (fabs(want) > 1 ? fabs(want) : 1);
    if (!ok) fails++;
    printf("%s %-46s %.12g (want %.12g)\n", ok ? "ok  " : "FAIL", name, got, want);
}
static void expect_msg(const char *name, const char *want)
{
    const char *m = C.message ? C.message : "(none)";
    int ok = strcmp(m, want) == 0;
    if (!ok) fails++;
    printf("%s %-46s \"%s\" (want \"%s\")\n", ok ? "ok  " : "FAIL", name, m, want);
}
/* X as shown: finishes a number still being typed */
static double x_now(void) { calc_finish_entry(&C); return C.stack[0].v; }
#define X (x_now())
#define Y (C.stack[1].v)
#define Z (C.stack[2].v)
#define T (C.stack[3].v)

int main(void)
{
    calc_init(&C);

    /* ---- 4-level stack, HP rules ---- */
    num(1); cmd(CMD_ENTER, 0); num(2); cmd(CMD_ENTER, 0); num(3); cmd(CMD_ENTER, 0); num(4);
    expect("stack T", T, 1); expect("stack X", X, 4);
    cmd(CMD_ADD, 0);
    expect("4+3 in X", X, 7); expect("T duplicated on drop", T, 1); expect("Z", Z, 1);
    cmd(CMD_ROLL_DOWN, 0);
    expect("R-down: X", X, 2); expect("R-down: T", T, 7);
    cmd(CMD_ROLL_UP, 0);
    expect("R-up restores X", X, 7);
    num(5); cmd(CMD_ENTER, 0); num(6);      /* ENTER then typing replaces the copy */
    expect("ENTER: Y", Y, 5); expect("ENTER: X", X, 6);

    /* ---- deeper stack ---- */
    cmd(CMD_CLEAR_STACK, 0);
    num(8); cmd(CMD_SET_STACK_SIZE, 0);
    expect("SSIZ 8", C.depth, 8);
    for (int i = 1; i <= 8; i++) { num(i); if (i < 8) cmd(CMD_ENTER, 0); }
    expect("level 8 holds 1", C.stack[7].v, 1);
    expect("T (level 4) holds 5", T, 5);
    cmd(CMD_ADD, 0); cmd(CMD_ADD, 0); cmd(CMD_ADD, 0);
    expect("three drops: X = 8+7+6+5", X, 26);
    expect("hidden level moved into T", T, 2);
    expect("top level duplicated", C.stack[7].v, 1);
    cmd(CMD_ROLL_DOWN, 0);
    expect("R-down through 8 levels: X", X, 4);
    expect("R-down: X went to level 8", C.stack[7].v, 26);
    cmd(CMD_GET_STACK_SIZE, 0);
    expect("SSZ?", X, 8);
    num(3); cmd(CMD_SET_STACK_SIZE, 0);
    expect("SSIZ 3 means the default 4", C.depth, 4);
    num(13); cmd(CMD_SET_STACK_SIZE, 0);
    expect_msg("SSIZ 13 is refused", "Err: Domain");

    /* ---- registers ---- */
    cmd(CMD_CLEAR_STACK, 0);
    num(12.5); cmd(CMD_STO, 1);                     /* STO A */
    num(3); cmd(CMD_STO, REG_I);                    /* STO i */
    cmd(CMD_RCL, 1);
    expect("RCL A", X, 12.5);
    num(1); cmd(CMD_STO, REG_I);                    /* i = 1 */
    cmd(CMD_RCL, REG_INDIRECT);
    expect("RCL (i) with i=1 is A", X, 12.5);
    num(26); cmd(CMD_STO, REG_I);
    num(99); cmd(CMD_STO, REG_INDIRECT);
    cmd(CMD_RCL, 26);
    expect("STO (i) with i=26 stored into Z", X, 99);
    num(2); cmd(CMD_STO_ADD, 1);                    /* STO+ A */
    cmd(CMD_RCL, 1);
    expect("STO+ A", X, 14.5);
    num(10); cmd(CMD_RCL_MUL, 1);
    expect("RCL* A", X, 145);
    num(7); cmd(CMD_XCH, 1);
    expect("x<> A: X gets A", X, 14.5);
    cmd(CMD_RCL, 1);
    expect("x<> A: A gets 7", X, 7);

    /* stat registers through (i) */
    cmd(CMD_STAT_CLEAR, 0);
    num(5); cmd(CMD_ENTER, 0); num(2); cmd(CMD_STAT_PLUS, 0);
    num(7); cmd(CMD_ENTER, 0); num(4); cmd(CMD_STAT_PLUS, 0);
    num(28); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect("register 028 is n", X, 2);
    num(29); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect("register 029 is Sx", X, 6);
    num(30); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect("register 030 is Sy", X, 12);
    num(33); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect("register 033 is Sxy", X, 38);
    expect("log fits still usable", C.stats.lnx_ok, 1);
    num(100); cmd(CMD_STO, REG_INDIRECT);           /* edit Sxy by hand */
    expect("editing a stat register disables log fits", C.stats.lnx_ok, 0);

    /* extra registers */
    num(41); cmd(CMD_STO, REG_I);
    num(123); cmd(CMD_STO, REG_INDIRECT);
    expect_msg("register 041 doesn't exist yet", "Err: Index");
    num(20); cmd(CMD_SET_REG_SIZE, 0);
    expect("RSIZ 20 means the default 40", C.extra_count, 0);
    num(141); cmd(CMD_SET_REG_SIZE, 0);
    expect_msg("RSIZ 141 is refused", "Err: Domain");
    num(60); cmd(CMD_SET_REG_SIZE, 0);
    expect("RSIZ 60 adds registers 041-060", C.extra_count, 20);
    num(41); cmd(CMD_STO, REG_I);
    num(123); cmd(CMD_STO, REG_INDIRECT);
    cmd(CMD_CLEAR, 0);
    cmd(CMD_RCL, REG_INDIRECT);
    expect("STO/RCL (i) in register 041", X, 123);
    num(60); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect_msg("register 060 is the last one (041-060)", "(none)");
    num(61); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect_msg("register 061 is past the end", "Err: Index");
    num(0); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect_msg("i = 0 is not a register", "Err: Index");
    num(41.9); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    expect("i = 41.9 uses its integer part, 041", X, 123);
    cmd(CMD_GET_REG_SIZE, 0);
    expect("RSZ? gives the total", X, 60);
    cmd(CMD_CLEAR_REGS, 0);
    cmd(CMD_RCL, 1);
    expect("CLRG cleared A", X, 0);
    expect("CLRG cleared register 041", C.extra[0].v, 0);
    expect("CLRG kept the stat data", C.stats.sum[SUM_N].v, 2);

    /* units travel with their values */
    cmd(CMD_CLEAR_STACK, 0);
    num(12); cmd(CMD_UNIT, 0);
    expect("unit attached to X", C.unit[0], 1);
    cmd(CMD_ENTER, 0);
    expect("ENTER copies the unit", C.unit[1], 1);
    cmd(CMD_SWAP, 0); cmd(CMD_DROP, 0);
    expect("units follow values through DROP", C.unit[0], 1);

    /* ---- NUM ---- */
    cmd(CMD_CLEAR_STACK, 0);
    num(91); cmd(CMD_PRIME, 0);
    expect("PRIM 91 = 7", X, 7);
    num(97); cmd(CMD_PRIME, 0);
    expect("PRIM 97 (prime) = 1", X, 1);
    num(4294967291.0); cmd(CMD_PRIME, 0);
    expect("PRIM 4294967291 (largest 32-bit prime) = 1", X, 1);
    num(4294967295.0); cmd(CMD_PRIME, 0);
    expect("PRIM 4294967295 = 3", X, 3);
    num(1); cmd(CMD_PRIME, 0);
    expect_msg("PRIM 1 is refused", "Err: Domain");
    num(3.14159265); cmd(CMD_ROUND_N, 2);
    expect("RNDN 2", X, 3.14);
    num(123456.789); cmd(CMD_ROUND_G, 3);
    expect("RNDG 3", X, 123000);
    cmd(CMD_DISP_FIX, 1);
    num(2.46); cmd(CMD_ROUND_FIX, 0);
    expect("RNDF in FIX 1", X, 2.5);
    cmd(CMD_DISP_FIX, 11);
    expect("FIX 11 is a real setting (HP-32SII range)", C.disp_digits, 11);
    cmd(CMD_DISP_FIX, 12);
    expect("FIX 12 (. 2) means as many digits as needed", C.disp_digits, -1);

    /* ---- CONV ---- */
    cmd(CMD_MODE_DEG, 0);
    num(4); cmd(CMD_ENTER, 0); num(3);          /* y = 4, x = 3 */
    cmd(CMD_TO_POL, 0);
    expect("->POL r", X, 5); expect("->POL angle", Y, 53.130102354156);
    cmd(CMD_TO_REC, 0);
    expect("->REC x", X, 3); expect("->REC y", Y, 4);
    num(1.3045); cmd(CMD_TO_HOURS, 0);
    expect("->HR 1.3045", X, 1.5125);
    cmd(CMD_TO_HMS, 0);
    expect("->HMS back", X, 1.3045);
    num(1.45); cmd(CMD_ENTER, 0); num(0.30); cmd(CMD_HMS_ADD, 0);
    expect("HMS+ 1:45 + 0:30 = 2:15", X, 2.15);
    num(0.5); cmd(CMD_SINH, 0);
    expect("SINH", X, sinh(0.5));

    /* ---- BASE ---- */
    cmd(CMD_CLEAR_STACK, 0);
    cmd(CMD_BASE_RADIX, 16);
    cmd(CMD_DIGIT, 15); cmd(CMD_DIGIT, 15);      /* FF */
    expect("HEX entry FF", X, 255);
    cmd(CMD_ENTER, 0); cmd(CMD_DIGIT, 1);
    cmd(CMD_ADD, 0);                             /* the + key does BAS+ */
    expect("HEX FF + 1 = 100", X, 256);
    num(8); cmd(CMD_SET_WORD_SIZE, 0);
    expect_msg("WSIZ 8", "Word Size Set");
    cmd(CMD_CLEAR_STACK, 0);
    cmd(CMD_DIGIT, 15); cmd(CMD_DIGIT, 15); cmd(CMD_ENTER, 0); cmd(CMD_DIGIT, 2);
    cmd(CMD_ADD, 0);
    expect("8-bit FF + 2 wraps to 1", X, 1);
    expect("... and sets carry", C.base.carry, 1);
    cmd(CMD_DIGIT, 15); cmd(CMD_DIGIT, 15); cmd(CMD_DIGIT, 15);
    expect("a third hex digit doesn't fit 8 bits", X, 255);
    cmd(CMD_CHS, 0);
    expect("(-) in HEX is NEG: FF -> 01", X, 1);
    cmd(CMD_BASE_RADIX, 2);
    cmd(CMD_DIGIT, 1); cmd(CMD_DIGIT, 2); cmd(CMD_DIGIT, 1);
    expect("BIN ignores the digit 2", X, 3);
    cmd(CMD_BASE_OP, BOP_CNTB);
    expect("CNTB 11 = 2", X, 2);
    /* 44-bit words */
    cmd(CMD_BASE_RADIX, 10); cmd(CMD_CLEAR_STACK, 0);
    num(5133136320.0); cmd(CMD_ENTER, 0); num(1);
    cmd(CMD_BASE_RADIX, 16);
    cmd(CMD_ADD, 0);
    expect_msg("33-bit value in a 32-bit word: BAS+", "Err: Word Size");
    cmd(CMD_BASE_RADIX, 10); num(44); cmd(CMD_SET_WORD_SIZE, 0);
    expect_msg("WSIZ 44", "Word Size Set");
    cmd(CMD_DROP, 0);                   /* drop the 44 */
    cmd(CMD_BASE_RADIX, 16);
    cmd(CMD_ADD, 0);
    expect("in 44 bits, 131F571C0 + 1 = 131F571C1", X, 5133136321.0);
    cmd(CMD_CLEAR_STACK, 0);
    for (int i = 0; i < 12; i++) cmd(CMD_ALPHA_LETTER, 5);   /* F x 12 */
    calc_finish_entry(&C);
    expect("typing F x 12 in HEX keeps 11 (44 bits)", X, 17592186044415.0);
    cmd(CMD_BASE_RADIX, 10);
    num(45); cmd(CMD_SET_WORD_SIZE, 0);
    expect_msg("WSIZ 45 is refused", "Err: Domain");
    num(12); cmd(CMD_SET_WORD_SIZE, 0);
    expect("WSIZ 12 (not a multiple of 8) is allowed", C.base.bits, 12);
    num(32); cmd(CMD_SET_WORD_SIZE, 0);

    /* ---- negative bases: the HP rule (MATH QA campaign, PR #1) ----
     * A root of a negative Y needs an odd whole-number index X; a power of
     * one a whole-number exponent. Rejected operations leave X, Y, the
     * rest of the stack and LASTX untouched. */
    {
        struct { const char *id; double y, x; int cmd; int ok; double want; } t[] = {
            /* valid: must keep working */
            { "MATH-010", 27, 3, CMD_XROOT_Y, 1, 3 },
            { "MATH-011", 81, 4, CMD_XROOT_Y, 1, 3 },
            { "MATH-012", -8, 3, CMD_XROOT_Y, 1, -2 },
            { "MATH-017", -8, -3, CMD_XROOT_Y, 1, -0.5 },
            { "MATH-092", -243, 5, CMD_XROOT_Y, 1, -3 },
            { "MATH-097", -243, -5, CMD_XROOT_Y, 1, -1.0 / 3 },
            { "MATH-094", 64, 6, CMD_XROOT_Y, 1, 2 },
            { "MATH-096", 81, -4, CMD_XROOT_Y, 1, 1.0 / 3 },
            { "MATH-100", 81, 0.8, CMD_XROOT_Y, 1, 243 },
            { "MATH-105", 0, 5, CMD_XROOT_Y, 1, 0 },
            { "MATH-106", 16, -0.5, CMD_XROOT_Y, 1, 0.00390625 },
            { "index -1", -5, -1, CMD_XROOT_Y, 1, -0.2 },
            /* invalid: Err: Domain */
            { "MATH-013", -16, 4, CMD_XROOT_Y, 0, 0 },
            { "MATH-015", -16, 2, CMD_XROOT_Y, 0, 0 },
            { "MATH-018", -16, -4, CMD_XROOT_Y, 0, 0 },
            { "MATH-090", -81, 4, CMD_XROOT_Y, 0, 0 },
            { "MATH-093", -64, 6, CMD_XROOT_Y, 0, 0 },
            { "MATH-095", -81, -4, CMD_XROOT_Y, 0, 0 },
            { "MATH-099", -81, 0.8, CMD_XROOT_Y, 0, 0 },
            { "MATH-031", -16, -0.8, CMD_XROOT_Y, 0, 0 },
            /* fractional indices: an error on the HP-32SII / DM32 */
            { "MATH-027 (HP rule)", -3, 0.5, CMD_XROOT_Y, 0, 0 },
            { "MATH-028 (HP rule)", -16, 1.5, CMD_XROOT_Y, 0, 0 },
            { "MATH-098 (HP rule)", -32, 2.5, CMD_XROOT_Y, 0, 0 },
            /* the ^ key */
            { "(-2)^3", -2, 3, CMD_POW, 1, -8 },
            { "(-2)^4", -2, 4, CMD_POW, 1, 16 },
            { "(-2)^-3", -2, -3, CMD_POW, 1, -0.125 },
            { "(-8)^0.4 (HP rule)", -8, 0.4, CMD_POW, 0, 0 },
            { "(-16)^0.25", -16, 0.25, CMD_POW, 0, 0 },
            { "2^0.5", 2, 0.5, CMD_POW, 1, 1.4142135623731 },
        };
        cmd(CMD_CLEAR_STACK, 0);
        for (unsigned i = 0; i < sizeof t / sizeof *t; i++) {
            char name[64];
            cmd(CMD_CLEAR_STACK, 0);
            num(7); cmd(CMD_ENTER, 0);          /* a deeper level to watch */
            num(t[i].y); cmd(CMD_ENTER, 0); num(t[i].x);
            calc_finish_entry(&C);
            C.lastx = mk(42);
            cmd((calc_cmd_t)t[i].cmd, 0);
            if (t[i].ok) {
                snprintf(name, sizeof name, "%s: Y=%g X=%g", t[i].id, t[i].y, t[i].x);
                expect(name, X, t[i].want);
                snprintf(name, sizeof name, "%s: stack dropped, LASTX = X", t[i].id);
                expect(name, Y == 7 && C.lastx.v == t[i].x, 1);
            } else {
                snprintf(name, sizeof name, "%s: Y=%g X=%g -> Err: Domain", t[i].id, t[i].y, t[i].x);
                expect_msg(name, "Err: Domain");
                snprintf(name, sizeof name, "%s: X, Y, Z and LASTX untouched", t[i].id);
                expect(name, X == t[i].x && Y == t[i].y && Z == 7 && C.lastx.v == 42, 1);
            }
        }
        cmd(CMD_CLEAR_STACK, 0);
        num(16); cmd(CMD_ENTER, 0); num(0); cmd(CMD_XROOT_Y, 0);
        expect_msg("MATH-101: index 0 -> Err: Division By 0", "Err: Division By 0");
        expect("MATH-101: X and Y untouched", X == 0 && Y == 16, 1);
        cmd(CMD_CLEAR_STACK, 0);
        num(0); cmd(CMD_ENTER, 0); num(-3); cmd(CMD_XROOT_Y, 0);
        expect_msg("MATH-103: 0, index -3 -> Err: Domain", "Err: Domain");
        expect("MATH-103: X and Y untouched", X == -3 && Y == 0, 1);
        cmd(CMD_CLEAR_STACK, 0);
        num(0); cmd(CMD_ENTER, 0); num(-1); cmd(CMD_XROOT_Y, 0);
        expect_msg("MATH-104: 0, index -1 -> Err: Domain", "Err: Domain");
    }

    /* ---- DATE objects through keystrokes ---- */
    cmd(CMD_CLEAR_STACK, 0);
    calc_execute(&C, CMD_DISP_FIX, 99);
    /* 2ND { 2024 , 3 , 14 ENTER */
    #define REC(...) do { const char *k_ = __VA_ARGS__; for (const char *p_ = k_; *p_; p_++) { \
        if (*p_ == '{') cmd(CMD_RECORD_OPEN, 0); else if (*p_ == ',') cmd(CMD_EE, 0); \
        else if (*p_ == '-') cmd(CMD_CHS, 0); else if (*p_ == '}') cmd(CMD_RECORD_CLOSE, 0); \
        else cmd(CMD_DIGIT, (uint8_t)(*p_ - '0')); } } while (0)
    #define XOBJ(name, want) do { calc_finish_entry(&C); char b_[40] = "(a number)"; \
        if (obj_is(&C.stack[0])) { dt_obj_t o_ = obj_unpack(&C.stack[0]); dt_format(b_, &o_, false); } \
        int ok_ = strcmp(b_, want) == 0; if (!ok_) fails++; \
        printf("%s %-46s %s (want %s)\n", ok_ ? "ok  " : "FAIL", name, b_, want); } while (0)
    REC("{1987,01,17"); cmd(CMD_ENTER, 0); num(15000); cmd(CMD_ADD, 0);
    XOBJ("reported birth date + 15000 days", "D{2028,2,11}");
    cmd(CMD_CLEAR_STACK, 0);
    REC("{1987,01,17"); cmd(CMD_ENTER, 0); REC("{2026,10,19"); cmd(CMD_SUB, 0);
    expect("1987-01-17 minus 2026-10-19 is days", X, -14520);
    expect("date subtraction remembers the original date in LASTX", obj_is(&C.lastx), 1);
    cmd(CMD_CLEAR_STACK, 0);
    REC("{2026,10,19"); cmd(CMD_ENTER, 0); REC("{1987,01,17"); cmd(CMD_SUB, 0);
    expect("date difference reverses sign in the other order", X, 14520);
    cmd(CMD_CLEAR_STACK, 0);
    REC("{2024,3,14"); cmd(CMD_ENTER, 0);
    XOBJ("typed Date (no closing brace)", "D{2024,3,14}");
    expect("ENTER copies the Date into Y", obj_is(&C.stack[1]), 1);
    num(20); cmd(CMD_ADD, 0);
    XOBJ("Date + 20 (the + key)", "D{2024,4,3}");
    REC("{2024,12,25"); cmd(CMD_SWAP, 0); cmd(CMD_SUB, 0);
    expect("Dec 25 - Apr 3 = 266 days", X, 266);
    REC("{2024,2,30"); cmd(CMD_ENTER, 0);
    expect_msg("Feb 30 keeps the entry open", "Err: Invalid");
    expect("... still typing", C.editing, 1);
    cmd(CMD_BACKSPACE, 0); cmd(CMD_BACKSPACE, 0); cmd(CMD_DIGIT, 2); cmd(CMD_DIGIT, 9);
    cmd(CMD_ENTER, 0);
    XOBJ("fixed to Feb 29 with DEL", "D{2024,2,29}");
    cmd(CMD_CLEAR_STACK, 0);
    REC("{2024,3,14,15,36,1,-7,0"); calc_finish_entry(&C);
    XOBJ("ZonedDateTime with a negative field", "DZ{2024,3,14,15,36,1,-7,0}");
    cmd(CMD_DATE_FN, DF_TO_ES);
    expect(">ES = 1710455761 (RPN83P's example)", X, 1710455761.0);
    cmd(CMD_DATE_FN, DF_ES_TO_DZ_APP);
    XOBJ("ES>@ at app zone UTC", "DZ{2024,3,14,22,36,1,0,0}");
    num(13); cmd(CMD_MUL, 0);
    XOBJ("x key converts to UTC+13", "DZ{2024,3,15,11,36,1,13,0}");
    cmd(CMD_RECIP, 0);
    XOBJ("1/x is DCUT: TimeZone in X", "TZ{13,0}");
    expect("... DateTime in Y", obj_is(&C.stack[1]), 1);
    cmd(CMD_DATE_FN, DF_LINK);
    XOBJ("DLNK puts them back together", "DZ{2024,3,15,11,36,1,13,0}");
    cmd(CMD_SQRT, 0);
    XOBJ("sqrt is DSHK", "DT{2024,3,15,11,36,1}");
    cmd(CMD_SQUARE, 0);
    XOBJ("x^2 is DEXD", "DZ{2024,3,15,11,36,1,0,0}");
    cmd(CMD_DATE_FN, DF_DOW);
    XOBJ("DOW", "DW{5}");
    cmd(CMD_LN, 0);
    expect_msg("LN of a DayOfWeek", "Err: Data Type");
    cmd(CMD_CHS, 0);
    expect_msg("CHS of an object", "Err: Data Type");

    /* menu-assisted entry and tags */
    cmd(CMD_CLEAR_STACK, 0);
    cmd(CMD_RECORD_TAG, OBJ_TIME); REC("15,36,1"); calc_finish_entry(&C);
    XOBJ("T{} menu key", "T{15,36,1}");
    cmd(CMD_ALPHA_LETTER, 'D' - 'A'); cmd(CMD_ALPHA_LETTER, 'W' - 'A'); REC("{4"); calc_finish_entry(&C);
    XOBJ("ALPHA D ALPHA W tag", "DW{4}");
    num(1); cmd(CMD_ALPHA_LETTER, 'D' - 'A'); cmd(CMD_DIGIT, 3); cmd(CMD_ALPHA_LETTER, 'M' - 'A');
    calc_finish_entry(&C);
    XOBJ("compact duration 1D3M", "DR{1,0,3,0}");
    cmd(CMD_RECORD_CLOSE, 0);
    XOBJ("2ND } outside a record is DROP", "DW{4}");

    /* registers hold objects; STO+ adds to them */
    cmd(CMD_CLEAR_STACK, 0);
    REC("{2024,3,14"); cmd(CMD_STO, 1);
    num(10); cmd(CMD_STO_ADD, 1);
    cmd(CMD_RCL, 1);
    XOBJ("STO A, then STO+ 10 days", "D{2024,3,24}");
    num(28); cmd(CMD_STO, REG_I); REC("{2024,3,14"); cmd(CMD_STO, REG_INDIRECT);
    expect_msg("an object can't go in a stat register", "Err: Data Type");

    /* guards */
    cmd(CMD_STAT_PLUS, 0);
    expect_msg("Sigma+ with a Date", "Err: Data Type");
    cmd(CMD_UNIT, 0);
    expect_msg("a unit on a Date", "Err: Data Type");
    num(32); cmd(CMD_SET_WORD_SIZE, 0);
    REC("{2024,3,14"); cmd(CMD_SET_STACK_SIZE, 0);
    expect_msg("SSIZ with a Date", "Err: Data Type");
    cmd(CMD_BASE_RADIX, 16);
    num(5); cmd(CMD_ADD, 0);
    XOBJ("in HEX, Date + 5 still adds days", "D{2024,3,19}");
    cmd(CMD_BASE_RADIX, 10);

    /* clock: the stand-in clock reads 2026-10-18 09:30:00 at UTC */
    cmd(CMD_CLEAR_STACK, 0);
    num(-7); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
    cmd(CMD_DATE_FN, DF_NOW_DZ);
    XOBJ("NWDZ at app zone UTC-07:00", "DZ{2026,10,18,2,30,0,-7,0}");
    cmd(CMD_DATE_FN, DF_NOW_DATE);
    XOBJ("NOWD (local date)", "D{2026,10,18}");
    REC("{2026,12,31,23,59,59,0,0"); cmd(CMD_DATE_FN, DF_SET_CLOCK);
    expect("SETC wrote the clock", mock_clock[0] == 2026 && mock_clock[1] == 12 && mock_clock[3] == 23, 1);

    /* The TI clock stays local; CTZ interprets it and ATZ selects app time. */
    {
        int saved[6];
        memcpy(saved, mock_clock, sizeof saved);
        const int local[6] = { 2026, 12, 31, 23, 30, 0 };
        memcpy(mock_clock, local, sizeof local);
        num(-7); cmd(CMD_DATE_FN, DF_SET_CLOCK_TZ);
        num(-7); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
        cmd(CMD_DATE_FN, DF_NOW_TIME);
        XOBJ("CTZ=ATZ preserves local clock time", "T{23,30,0}");
        cmd(CMD_DATE_FN, DF_NOW_DZ);
        XOBJ("NWDZ carries the application offset", "DZ{2026,12,31,23,30,0,-7,0}");
        cmd(CMD_DATE_FN, DF_NOW_DZ_UTC);
        XOBJ("NWUT crosses into next UTC year", "DZ{2027,1,1,6,30,0,0,0}");
        cmd(CMD_DATE_FN, DF_NOW);
        double instant = X;
        num(9); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
        cmd(CMD_DATE_FN, DF_NOW_TIME);
        XOBJ("ATZ changes application time", "T{15,30,0}");
        cmd(CMD_DATE_FN, DF_NOW_DATE);
        XOBJ("ATZ changes application date at midnight", "D{2027,1,1}");
        cmd(CMD_DATE_FN, DF_NOW_DZ);
        XOBJ("NWDZ uses new ATZ", "DZ{2027,1,1,15,30,0,9,0}");
        cmd(CMD_DATE_FN, DF_NOW);
        expect("NOW epoch seconds independent of ATZ", X, instant);
        expect("clock reads and zone setters preserve hardware time",
               !memcmp(mock_clock, local, sizeof local) && C.clock_tz == -28, 1);
        const int fractional[6] = { 2027, 1, 1, 0, 15, 0 };
        memcpy(mock_clock, fractional, sizeof fractional);
        num(5.5); cmd(CMD_DATE_FN, DF_SET_CLOCK_TZ);
        num(-3.5); cmd(CMD_DATE_FN, DF_SET_APP_TZ);
        cmd(CMD_DATE_FN, DF_NOW_DZ);
        XOBJ("fractional zones cross previous year", "DZ{2026,12,31,15,15,0,-3,-30}");
        cmd(CMD_DATE_FN, DF_NOW_DZ_UTC);
        XOBJ("UTC removes fractional clock offset", "DZ{2026,12,31,18,45,0,0,0}");
        memcpy(mock_clock, saved, sizeof saved);
        C.clock_tz = 0;
        C.app_tz = -28;
    }

    /* epochs */
    cmd(CMD_DATE_FN, DF_EPOCH_NTP);
    REC("{2024,3,14,15,36,1,-7,0"); cmd(CMD_DATE_FN, DF_TO_ES);
    expect("NTP epoch seconds = 3919444561", X, 3919444561.0);
    cmd(CMD_DATE_FN, DF_EPOCH_UNIX);

    /* Result bookkeeping: unary, binary, register, BASE, and statistics
     * commands must keep their distinct stack-lift and LASTX rules. */
    {
        static const struct {
            calc_cmd_t command;
            uint8_t arg;
            double result;
            bool drops, lift;
        } cases[] = {
            { CMD_SQUARE, 0, 4, false, true },
            { CMD_ADD, 0, 12, true, true },
            { CMD_PERCENT, 0, 0.2, false, true },
            { CMD_PERCENT_CHANGE, 0, -80, false, true },
            { CMD_RCL_ADD, REG_A, 5, false, true },
            { CMD_BASE_OP, BOP_ADD, 12, true, true },
            { CMD_BASE_OP, BOP_NOT, 4294967293.0, false, true },
            { CMD_STAT_PLUS, 0, 1, false, false },
        };
        for (int depth = STACK_MIN; depth <= STACK_MAX; depth += STACK_MAX - STACK_MIN)
            for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
                calc_init(&C);
                C.depth = (uint8_t)depth;
                C.lift = false;
                for (int k = 0; k < depth; k++) {
                    C.stack[k] = mk(k == 0 ? 2 : k == 1 ? 10 : 100 + k);
                    C.unit[k] = k < 2 ? 0 : (uint8_t)(k + 1);
                }
                C.regs[0] = mk(3);
                cmd(cases[i].command, cases[i].arg);
                expect("result matches operation", C.stack[0].v, cases[i].result);
                expect("LASTX keeps the original X and unit", C.lastx.v == 2 && C.unit_lastx == 0, 1);
                expect("result has no unit", C.unit[0], 0);
                expect("operation keeps its lift rule", C.lift, cases[i].lift);
                expect("Y and its unit follow the drop rule",
                       C.stack[1].v == (cases[i].drops ? 102 : 10) &&
                       C.unit[1] == (cases[i].drops ? 3 : 0), 1);
                expect("top stack level stays duplicated", C.stack[depth - 1].v, 100 + depth - 1);
                expect("no unexpected result error", C.message == NULL, 1);
            }

        calc_init(&C);
        C.stack[0] = mk(0);
        C.stack[1] = mk(10);
        C.lastx = mk(42);
        C.unit[0] = 3;
        C.unit[1] = 4;
        C.unit_lastx = 3;
        C.lift = false;
        cmd(CMD_DIV, 0);
        expect_msg("failed result reports division by zero", "Err: Division By 0");
        expect("failed result preserves values, tags, LASTX and lift",
               C.stack[0].v == 0 && C.stack[1].v == 10 && C.lastx.v == 42 &&
               C.unit[0] == 3 && C.unit[1] == 4 && C.unit_lastx == 3 && !C.lift, 1);
    }

    /* DATE conversion boundaries: ordinary epoch/duration functions need
     * whole values, whereas timezone commands also accept quarter hours. */
    {
        static const double hours[] = { -23.75, -7.75, -0.25, 0, 5.5, 23.75 };
        for (unsigned i = 0; i < sizeof hours / sizeof hours[0]; i++) {
            calc_init(&C);
            C.stack[0] = mk(hours[i]);
            cmd(CMD_DATE_FN, DF_HR_TO_TZ);
            dt_obj_t zone = obj_unpack(&C.stack[0]);
            expect("hours convert exactly to quarter-hour offsets",
                   obj_is(&C.stack[0]) && zone.type == OBJ_TZ &&
                   dt_tz_quarters(&zone) == (int)(hours[i] * 4), 1);
            cmd(CMD_DATE_FN, DF_SET_APP_TZ);
            expect("object offset sets application timezone", C.app_tz, hours[i] * 4);
            C.stack[0] = mk(hours[i]);
            cmd(CMD_DATE_FN, DF_SET_CLOCK_TZ);
            expect("numeric offset sets clock timezone", C.clock_tz, hours[i] * 4);
        }
        calc_init(&C);
        C.stack[0] = mk(1.1);
        cmd(CMD_DATE_FN, DF_SET_APP_TZ);
        expect_msg("fractional quarter-hour offset is rejected", "Err: Domain");
        expect("rejected timezone keeps setting and X", C.app_tz == 0 && C.stack[0].v == 1.1, 1);
        C.stack[0] = mk(86401);
        cmd(CMD_DATE_FN, DF_ES_TO_D);
        XOBJ("whole epoch seconds still convert to a date", "D{1970,1,2}");
        C.stack[0] = mk(0.25);
        cmd(CMD_DATE_FN, DF_SECONDS_TO_DR);
        expect_msg("duration seconds still require whole values", "Err: Domain");
    }

    printf("%d failures\n", fails);
    return fails != 0;
}
