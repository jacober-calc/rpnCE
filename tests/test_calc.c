/* Tests for the calculator core: stack, registers, (i), units on the stack.
 * Runs the real calc.c on a PC with stand-ins for the OS number routines:
 *   gcc -DRPNCE_HOST_TEST -Itests/mock -Isrc/core -o test_calc tests/test_calc.c src/core/calc.c \
 *       src/core/mathx.c src/core/stats.c src/core/tvm.c src/core/units.c \
 *       tests/units_data.c src/core/base.c -lm && ./test_calc
 */
#include <stdio.h>
#include <string.h>
#include <ti/real.h>
#include <ti/error.h>
#include "calc.h"
#include "dateobj.h"

real_t mock_fin[7];
jmp_buf mock_jmp;
real_t os_RealDiv(const real_t *a, const real_t *b)
{ if (b->v == 0) os_ThrowError(OS_E_DIVBY0); return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a)
{ if (a->v <= 0) os_ThrowError(OS_E_DOMAIN); return mk(log(a->v)); }
static long long g(long long a, long long b) { while (b) { long long t = a % b; a = b; b = t; } return a; }
real_t os_RealGcd(const real_t *a, const real_t *b) { return mk((double)g((long long)a->v, (long long)b->v)); }
real_t os_RealLcm(const real_t *a, const real_t *b)
{ long long x = (long long)a->v, y = (long long)b->v; return mk((double)(x / g(x, y) * y)); }
void format_real(char *out, const real_t *v, uint8_t style, int8_t digits, int maxd, int maxe, bool group)
{ (void)style; (void)digits; (void)maxd; (void)maxe; (void)group; sprintf(out, "%.14g", v->v); }

/* decimal rounding stand-ins (the real ones work on the calculator's digits) */
real_t round_decimals(const real_t *v, int n) { double p = pow(10, n); return mk(round(v->v * p) / p); }
real_t round_significant(const real_t *v, int n)
{
    if (v->v == 0) return *v;
    int e = (int)floor(log10(fabs(v->v)));
    return round_decimals(v, n - 1 - e);
}

/* a stand-in hardware clock */
static int clk[6] = { 2026, 10, 18, 9, 30, 0 };
bool sys_clock_read(int *y, int *mo, int *d, int *h, int *mi, int *s)
{ *y = clk[0]; *mo = clk[1]; *d = clk[2]; *h = clk[3]; *mi = clk[4]; *s = clk[5]; return true; }
void sys_clock_write(int y, int mo, int d, int h, int mi, int s)
{ clk[0] = y; clk[1] = mo; clk[2] = d; clk[3] = h; clk[4] = mi; clk[5] = s; }

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
    num(32); cmd(CMD_BASE_RADIX, 10); num(32); cmd(CMD_SET_WORD_SIZE, 0);
    cmd(CMD_BASE_RADIX, 10);

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
    expect("SETC wrote the clock", clk[0] == 2026 && clk[1] == 12 && clk[3] == 23, 1);

    /* epochs */
    cmd(CMD_DATE_FN, DF_EPOCH_NTP);
    REC("{2024,3,14,15,36,1,-7,0"); cmd(CMD_DATE_FN, DF_TO_ES);
    expect("NTP epoch seconds = 3919444561", X, 3919444561.0);
    cmd(CMD_DATE_FN, DF_EPOCH_UNIX);

    printf("%d failures\n", fails);
    return fails != 0;
}
