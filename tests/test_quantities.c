/* Quantity arithmetic, register metadata, VARS and numeric command regressions. */
#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "units.h"
#include "dateobj.h"
#include "catalog.h"

static calc_t c;
static int fails;
static void check(const char *name, bool pass)
{
    if (!pass) fails++;
    printf("%s %s\n", pass ? "ok  " : "FAIL", name);
}
static bool near(double a, double b)
{ return isfinite(a) && (b == 0 ? fabs(a) : fabs((a - b) / b)) <= 1e-10; }
static uint8_t unit(const char *name)
{
    for (uint8_t i = 0; i < UNIT_COUNT; i++)
        if (!strcmp(unit_label(i), name)) return i + 1;
    fprintf(stderr, "missing test unit: %s\n", name);
    return 0;
}
static void cmd(calc_cmd_t k, uint8_t arg) { calc_execute(&c, k, arg); }
static void number(unsigned n)
{
    char text[16];
    sprintf(text, "%u", n);
    for (char *p = text; *p; p++) cmd(CMD_DIGIT, (uint8_t)(*p - '0'));
}
static void quantity(unsigned n, const char *label)
{ number(n); cmd(CMD_UNIT, unit(label) - 1); }
static void pair(double y, uint8_t uy, double x, uint8_t ux)
{
    calc_init(&c);
    c.stack[0] = mk(x); c.unit[0] = ux;
    c.stack[1] = mk(y); c.unit[1] = uy;
    c.stack[2] = mk(123); c.unit[2] = unit("ft");
    c.lastx = mk(456); c.unit_lastx = unit("in");
    c.lift = false;
}
static void refused(const char *name, calc_cmd_t k, uint8_t arg, const char *message)
{
    calc_t before = c;
    cmd(k, arg);
    check(name, c.message && !strcmp(c.message, message) &&
          !memcmp(before.stack, c.stack, sizeof c.stack) &&
          !memcmp(before.unit, c.unit, sizeof c.unit) &&
          !memcmp(&before.lastx, &c.lastx, sizeof c.lastx) &&
          before.unit_lastx == c.unit_lastx && before.lift == c.lift &&
          !memcmp(before.regs, c.regs, sizeof c.regs) &&
          !memcmp(before.reg_unit, c.reg_unit, sizeof c.reg_unit));
}

static void numbers(void)
{
    pair(99, 0, 1981, 0);
    cmd(CMD_PRIME, 0);
    check("PRIM 1981 pushes 7 and keeps 1981 in Y even with lift disabled",
          c.stack[0].v == 7 && c.stack[1].v == 1981 && c.stack[2].v == 99 &&
          !c.unit[0] && !c.unit[1] && c.lastx.v == 1981 && c.lift && !c.message);
    cmd(CMD_DIV, 0);
    check("PRIM then division removes a factor", c.stack[0].v == 283);
    cmd(CMD_PRIME, 0);
    check("PRIM of a prime pushes 1", c.stack[0].v == 1 && c.stack[1].v == 283);
    pair(0, 0, 2122438477, 0);
    const double factors[] = { 53, 4001, 1 };
    for (unsigned i = 0; i < 3; i++) {
        cmd(CMD_PRIME, 0);
        check("manual PRIM / sequence", c.stack[0].v == factors[i] && !c.message);
        cmd(CMD_DIV, 0);
    }
    pair(1, 0, 1, 0);
    refused("PRIM rejects 1 without changing values", CMD_PRIME, 0, "Err: Domain");
    const double inputs[][3] = { {6,8,24}, {8,6,24}, {21,6,42}, {6,21,42},
        {0,6,0}, {6,0,0}, {0,0,0}, {12,12,12}, {1000000,999999,999999000000.0} };
    for (unsigned i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        pair(inputs[i][0], 0, inputs[i][1], 0);
        cmd(CMD_LCM, 0);
        check("LCM computes the multiple and drops Y", near(c.stack[0].v, inputs[i][2]) &&
              c.stack[1].v == 123 && c.lastx.v == inputs[i][1] && c.lift && !c.message);
    }
    pair(6, 0, -8, 0);
    refused("LCM still rejects negative integers", CMD_LCM, 0, "Err: Domain");
}

static void quantities(void)
{
    calc_init(&c);
    quantity(2, "kWh"); quantity(2000, "kcal");
    cmd(CMD_PERCENT_CHANGE, 0);
    check("exact reported 2 kWh, 2000 kcal %CH example",
          near(c.stack[0].v, 16.222222222222) && !c.unit[0] &&
          c.stack[1].v == 2 && c.unit[1] == unit("kWh") &&
          c.lastx.v == 2000 && c.unit_lastx == unit("kcal") && !c.message);
    cmd(CMD_LASTX, 0);
    check("LASTX recalls the original quantity", c.stack[0].v == 2000 && c.unit[0] == unit("kcal"));

    pair(6, unit("ft"), 2, unit("in"));
    cmd(CMD_ADD, 0);
    check("addition converts X to Y's unit", near(c.stack[0].v, 6 + 2.0/12) &&
          c.unit[0] == unit("ft") && c.unit_lastx == unit("in") && !c.message);
    number(3); cmd(CMD_DIV, 0); cmd(CMD_UNIT, unit("in") - 1);
    check("arithmetic quantities can still be converted", near(c.stack[0].v, 74.0/3) && c.unit[0] == unit("in"));
    pair(6, unit("ft"), 2, unit("in")); cmd(CMD_SUB, 0);
    check("subtraction converts compatible units", near(c.stack[0].v, 6 - 2.0/12) && c.unit[0] == unit("ft"));
    pair(6, unit("ft"), 2, unit("in")); cmd(CMD_DIV, 0);
    check("quantity ratio returns a scalar", near(c.stack[0].v, 36) && !c.unit[0]);
    pair(6, unit("ft"), 25, 0); cmd(CMD_PERCENT, 0);
    check("percent of a quantity keeps Y and its unit", c.stack[0].v == 1.5 &&
          c.unit[0] == unit("ft") && c.stack[1].v == 6 && c.unit[1] == unit("ft"));
    pair(6, unit("ft"), 2, 0); cmd(CMD_MUL, 0);
    check("quantity times scalar keeps unit", c.stack[0].v == 12 && c.unit[0] == unit("ft"));
    pair(2, 0, 6, unit("ft")); cmd(CMD_MUL, 0);
    check("scalar times quantity keeps unit", c.stack[0].v == 12 && c.unit[0] == unit("ft"));
    pair(6, unit("ft"), 80, unit("in")); cmd(CMD_MIN, 0);
    check("MIN compares converted values", c.stack[0].v == 6 && c.unit[0] == unit("ft"));
    pair(6, unit("ft"), 80, unit("in")); cmd(CMD_MAX, 0);
    check("MAX compares converted values", near(c.stack[0].v, 80.0/12) && c.unit[0] == unit("ft"));
    pair(7, unit("ft"), 24, unit("in")); cmd(CMD_MOD, 0);
    check("MOD uses compatible units", near(c.stack[0].v, 1) && c.unit[0] == unit("ft"));
    pair(0, 0, -7, unit("ft")); cmd(CMD_ABS, 0);
    check("ABS keeps unit", c.stack[0].v == 7 && c.unit[0] == unit("ft"));
    cmd(CMD_CHS, 0); cmd(CMD_SIGN, 0);
    check("SIGN produces a scalar", c.stack[0].v == -1 && !c.unit[0]);
    pair(2, unit("kW"), 2000, unit("kcal"));
    refused("power and energy cannot be compared", CMD_PERCENT_CHANGE, 0, "Err: Unit Mismatch");
    pair(6, unit("ft"), 2, 0);
    refused("quantity plus scalar is refused", CMD_ADD, 0, "Err: Data Type");
    pair(6, unit("ft"), 2, unit("in"));
    refused("compound units are refused", CMD_MUL, 0, "Err: Data Type");
    refused("integer LCM refuses quantities", CMD_LCM, 0, "Err: Data Type");
    refused("BASE operations refuse quantities", CMD_BASE_OP, BOP_ADD, "Err: Data Type");
    refused("statistics refuse quantities", CMD_STAT_PLUS, 0, "Err: Data Type");
    pair(6, unit("ft"), 0, unit("in"));
    refused("zero divisor leaves quantities and LASTX intact", CMD_DIV, 0, "Err: Division By 0");
    pair(0, unit("ft"), 2, unit("in"));
    refused("zero initial quantity cannot have a percentage change", CMD_PERCENT_CHANGE, 0, "Err: Division By 0");
    pair(6, 0, 2, unit("ft"));
    refused("inverse units are refused", CMD_DIV, 0, "Err: Data Type");
    refused("settings refuse quantity values", CMD_SET_STACK_SIZE, 0, "Err: Data Type");
    refused("TVM refuses quantity values", CMD_TVM_VAR, TVM_N, "Err: Data Type");
    refused("DATE conversions refuse quantities", CMD_DATE_FN, DF_SECONDS_TO_DR, "Err: Data Type");
    refused("DATE year checks refuse quantities", CMD_DATE_FN, DF_LEAP, "Err: Data Type");
    refused("DATE time zones refuse quantities", CMD_DATE_FN, DF_SET_APP_TZ, "Err: Data Type");
    pair(6, unit("Lkm"), 2.7, unit("Lkm"));
    refused("FUEL arithmetic is refused", CMD_ADD, 0, "Err: Data Type");
    cmd(CMD_NEAR, 0);
    check("nonlinear units can be rounded", c.stack[0].v == 3 && c.unit[0] == unit("Lkm"));
    uint8_t temp = 0;
    for (uint8_t i = 0; i < UNIT_COUNT; i++) if (unit_dim(i) == DIM_TEMP) { temp = i + 1; break; }
    pair(6, temp, 2.7, temp);
    refused("TEMP arithmetic is refused", CMD_SUB, 0, "Err: Data Type");
    cmd(CMD_FLOOR, 0);
    check("temperature rounding keeps unit", c.stack[0].v == 2 && c.unit[0] == temp);
    pair(6, unit("ft"), 2, unit("in"));
    c.base.radix = 16;
    cmd(CMD_ADD, 0);
    check("quantities use unit arithmetic in BASE mode", near(c.stack[0].v, 6 + 2.0/12) && c.unit[0] == unit("ft"));
    pair(6, 0, 2, unit("ft"));
    dt_obj_t date = { .type = OBJ_DATE, .year = 2026, .month = 10, .day = 18 };
    c.stack[1] = obj_pack(&date);
    refused("DATE arithmetic refuses quantities", CMD_ADD, 0, "Err: Data Type");
    refused("percent change cannot treat DATE as a number", CMD_PERCENT_CHANGE, 0, "Err: Data Type");
}

static void registers(void)
{
    pair(0, 0, 6, unit("ft"));
    cmd(CMD_STO, REG_A);
    check("STO A stores the full quantity", c.regs[0].v == 6 && c.reg_unit[0] == unit("ft") && !c.lift);
    quantity(2, "in"); cmd(CMD_STO_ADD, REG_A);
    check("STO+ converts to the stored unit", near(c.regs[0].v, 6 + 2.0/12) &&
          c.reg_unit[0] == unit("ft") && c.stack[0].v == 2 && c.unit[0] == unit("in"));
    cmd(CMD_RCL_ADD, REG_A);
    check("RCL+ converts to X's unit", near(c.stack[0].v, 76) && c.unit[0] == unit("in") &&
          c.lastx.v == 2 && c.unit_lastx == unit("in"));
    cmd(CMD_XCH, REG_A);
    check("X<> exchanges both units and values", near(c.stack[0].v, 6 + 2.0/12) &&
          c.unit[0] == unit("ft") && near(c.regs[0].v, 76) && c.reg_unit[0] == unit("in"));
    cmd(CMD_ENTER, 0); cmd(CMD_RCL, REG_A);
    check("RCL after ENTER replaces X with a quantity", near(c.stack[0].v, 76) && c.unit[0] == unit("in") &&
          near(c.stack[1].v, 6 + 2.0/12) && c.unit[1] == unit("ft"));
    number(0); cmd(CMD_UNIT, unit("ft") - 1); cmd(CMD_STO, 2);
    catalog_t cat = {0};
    check("VARS includes zero quantities and exposes their units", catalog_open(&cat, &c) &&
          cat.items[0] == REG_A && cat.items[1] == 2 && catalog_unit(&c, 2) == unit("ft"));
    refused("statistics registers cannot store quantities", CMD_STO, REG_STAT_FIRST, "Err: Data Type");
    refused("statistics registers cannot exchange quantities", CMD_XCH, REG_STAT_FIRST, "Err: Data Type");
    number(9); cmd(CMD_STO, REG_A);
    check("overwriting with a scalar clears the old unit", c.regs[0].v == 9 && !c.reg_unit[0]);
    number(42); cmd(CMD_SET_REG_SIZE, 0);
    quantity(5, "ft"); cmd(CMD_STO, REG_EXTRA_FIRST);
    number(REG_EXTRA_FIRST); cmd(CMD_STO, REG_I); cmd(CMD_RCL, REG_INDIRECT);
    check("indirect recall of extra register keeps unit", c.stack[0].v == 5 && c.unit[0] == unit("ft"));
    cmd(CMD_STO, REG_I);
    refused("quantity cannot be used as an indirect index", CMD_RCL, REG_INDIRECT, "Err: Data Type");
    dt_obj_t date = { .type = OBJ_DATE, .year = 2026, .month = 10, .day = 18 };
    c.regs[REG_I - REG_A] = obj_pack(&date); c.reg_unit[REG_I - REG_A] = 0;
    refused("DATE cannot be used as an indirect index", CMD_RCL, REG_INDIRECT, "Err: Data Type");
    number(40); cmd(CMD_SET_REG_SIZE, 0);
    number(42); cmd(CMD_SET_REG_SIZE, 0); cmd(CMD_RCL, REG_EXTRA_FIRST);
    check("shrinking and growing registers clears removed values and units", c.stack[0].v == 0 && !c.unit[0]);
    quantity(6, "in"); cmd(CMD_STO, REG_EXTRA_FIRST); cmd(CMD_CLEAR_REGS, 0);
    check("CLRG clears all register units", !c.reg_unit[1] && !c.reg_unit[REG_I-1] && !c.extra_unit[0]);
}

static void tvm_catalog(void)
{
    static const char *const names[] = { "N", "I%YR", "PV", "PMT", "FV", "P/YR", "C/YR" };
    const double values[] = { 360, 6.5, -200000, 1264.136, 0, 12, 12 };
    real_t saved[TVM_VAR_COUNT];
    for (uint8_t v = 0; v < TVM_VAR_COUNT; v++)
        saved[v] = *tvm_var(v) = mk(values[v]);
    calc_init(&c);
    calc_t before = c;
    catalog_t cat = {0};
    check("VARS includes all seven TVM values, including zero FV",
          catalog_open(&cat, &c) && cat.count == TVM_VAR_COUNT + 1);
    check("zero LASTX is always the final VARS entry",
          cat.items[cat.count - 1] == CAT_LASTX && catalog_value(&c, CAT_LASTX)->v == 0);
    check("opening VARS does not change calculator state", !memcmp(&before, &c, sizeof c));
    for (uint8_t v = 0; v < TVM_VAR_COUNT; v++) {
        uint8_t item = cat.items[v];
        char name[12];
        catalog_name(item, name);
        check("TVM names, live values and scalar unit metadata agree",
              !strcmp(name, names[v]) && catalog_value(&c, item) == tvm_var(v) &&
              near(catalog_value(&c, item)->v, values[v]) && !catalog_unit(&c, item));
        for (int lift = 0; lift < 2; lift++) {
            pair(10, unit("in"), 3, unit("ft"));
            c.lift = lift != 0;
            c.last_cmd = CMD_TVM_VAR;  /* must recall, even after another TVM key */
            catalog_recall(&c, item);
            check("catalog TVM recall obeys lift rules without invoking the solver",
                  near(c.stack[0].v, values[v]) && !c.unit[0] &&
                  c.stack[1].v == (lift ? 3 : 10) &&
                  c.unit[1] == unit(lift ? "ft" : "in") &&
                  c.lastx.v == 456 && c.unit_lastx == unit("in") && c.lift && !c.message);
        }
    }
    bool unchanged = true;
    for (uint8_t v = 0; v < TVM_VAR_COUNT; v++)
        unchanged = unchanged && !memcmp(tvm_var(v), &saved[v], sizeof(real_t));
    check("catalog reads and recalls preserve all shared TVM variables", unchanged);
    *tvm_var(TVM_PV) = mk(-150000);
    check("catalog reads live TI TVM values", catalog_value(&c, CAT_TVM_FIRST + TVM_PV)->v == -150000);

    pair(10, 0, 3, 0);
    c.regs[0] = mk(6); c.reg_unit[0] = unit("ft");
    catalog_recall(&c, REG_A);
    check("catalog ordinary-register recall still carries units", c.stack[0].v == 6 && c.unit[0] == unit("ft"));
    catalog_recall(&c, CAT_LASTX);
    check("catalog LASTX recall still carries units", c.stack[0].v == 456 && c.unit[0] == unit("in"));

    calc_init(&c);
    c.extra_count = REG_EXTRA_MAX;
    for (int i = 0; i < REG_BASE_COUNT; i++) c.regs[i] = mk(i + 1);
    for (int i = 0; i < REG_EXTRA_MAX; i++) c.extra[i] = mk(i + 1);
    for (int i = 0; i < SUM_COUNT; i++) c.stats.sum[i] = mk(i + 1);
    c.lastx = mk(1);
    catalog_open(&cat, &c);
    check("full catalog fits all 140 registers, LASTX and seven TVM values",
          cat.count == 148 && cat.count == sizeof cat.items &&
          cat.items[139] == 140 && cat.items[140] == CAT_TVM_FIRST &&
          cat.items[146] == CAT_TVM_LAST && cat.items[147] == CAT_LASTX);
    catalog_move(&cat, -1);
    check("catalog wraps up to LASTX and scrolls it into view",
          cat.sel == 147 && cat.top == 144 && cat.items[cat.sel] == CAT_LASTX);
    catalog_move(&cat, 1);
    check("catalog wraps back to the first register", !cat.sel && !cat.top);
}

static void unit_matrix(void)
{
    static const calc_cmd_t rounding[] = { CMD_INT_PART, CMD_FRAC_PART, CMD_FLOOR,
        CMD_CEIL, CMD_NEAR, CMD_ROUND_FIX, CMD_ROUND_N, CMD_ROUND_G };
    static const calc_cmd_t arithmetic[] = { CMD_ADD, CMD_SUB, CMD_MUL, CMD_DIV,
        CMD_MOD, CMD_MIN, CMD_MAX, CMD_PERCENT, CMD_PERCENT_CHANGE, CMD_ABS, CMD_SIGN };
    for (uint8_t i = 0; i < UNIT_COUNT; i++) {
        uint8_t tag = i + 1;
        uint8_t dimension = unit_dim(i);
        pair(0, 0, 123.25, tag);
        cmd(CMD_UNIT, unit_base_of(dimension)); cmd(CMD_UNIT, i);
        check("each unit converts to its base and back", !c.message &&
              near(c.stack[0].v, 123.25) && c.unit[0] == tag);
        cmd(CMD_STO, REG_A); cmd(CMD_CLEAR, 0); cmd(CMD_RCL, REG_A);
        check("each unit survives ordinary register storage and recall",
              !c.message && near(c.stack[0].v, 123.25) && c.unit[0] == tag);
        for (unsigned k = 0; k < sizeof rounding / sizeof *rounding; k++) {
            pair(0, 0, 12.75, 0); cmd(rounding[k], 2);
            double expected = c.stack[0].v;
            pair(0, 0, 12.75, tag); cmd(rounding[k], 2);
            check("each unit supports documented numeric rounding",
                  !c.message && near(c.stack[0].v, expected) && c.unit[0] == tag);
        }
        if (dimension == DIM_TEMP || dimension == DIM_FUEL) {
            for (unsigned k = 0; k < sizeof arithmetic / sizeof *arithmetic; k++) {
                pair(6, tag, 2, tag);
                refused("TEMP/FUEL arithmetic preserves state on refusal",
                        arithmetic[k], 0, "Err: Data Type");
            }
        } else {
            pair(6, tag, 2, tag); cmd(CMD_ADD, 0);
            check("each linear unit supports quantity addition",
                  !c.message && near(c.stack[0].v, 8) && c.unit[0] == tag);
            pair(6, tag, 2, tag); cmd(CMD_DIV, 0);
            check("each linear unit supports a dimensionless ratio",
                  !c.message && near(c.stack[0].v, 3) && !c.unit[0]);
        }
    }
}

int main(void)
{
    numbers(); quantities(); registers(); tvm_catalog(); unit_matrix();
    printf("%d failures\n", fails);
    return fails != 0;
}
