/* Compare the original and refactored cores using the same host OS mocks.
 * Run with sh tests/check_core_equivalence.sh [baseline-commit]. */
#include <stdio.h>
#include <string.h>
#include "calc.h"
#include "dateobj.h"
#include "mathx.h"
#include "units.h"
#include "mock_calc_runtime.h"

void original_calc_init(calc_t *c);
void original_calc_execute(calc_t *c, calc_cmd_t cmd, uint8_t arg);

static bool same_real(const real_t *a, const real_t *b)
{
    if (obj_is(a) || obj_is(b))
        return a->sign == b->sign && a->exp == b->exp &&
               memcmp(a->mant, b->mant, sizeof a->mant) == 0;
    return a->v == b->v || (isnan(a->v) && isnan(b->v));
}

static bool same_object(const dt_obj_t *a, const dt_obj_t *b)
{
    return a->type == b->type && a->year == b->year && a->month == b->month &&
           a->day == b->day && a->hour == b->hour && a->minute == b->minute &&
           a->second == b->second && a->tz_hour == b->tz_hour && a->tz_minute == b->tz_minute;
}

static bool same_state(const calc_t *a, const calc_t *b)
{
#define EQ(field) if (a->field != b->field) return false
    EQ(depth); EQ(extra_count); EQ(lift); EQ(degrees); EQ(disp_style); EQ(disp_digits);
    EQ(editing); EQ(entry_len); EQ(last_cmd); EQ(unit_lastx); EQ(date_as_string);
    EQ(epoch); EQ(app_tz); EQ(clock_tz);
    EQ(base.radix); EQ(base.bits); EQ(base.carry);
    EQ(tvm.begin); EQ(tvm.iter_max); EQ(tvm.iyr1_set); EQ(tvm.iyr2_set); EQ(tvm.iter_max_set);
    EQ(stats.linear_only); EQ(stats.lnx_ok); EQ(stats.lny_ok); EQ(stats.model);
#undef EQ
    if (strcmp(a->entry, b->entry) || strcmp(a->show_buf, b->show_buf) ||
        (a->message == NULL) != (b->message == NULL) ||
        (a->message && strcmp(a->message, b->message)) ||
        !same_object(&a->custom_epoch, &b->custom_epoch) ||
        !same_real(&a->lastx, &b->lastx) || !same_real(&a->tvm.iyr1, &b->tvm.iyr1) ||
        !same_real(&a->tvm.iyr2, &b->tvm.iyr2)) return false;
    for (int i = 0; i < STACK_MAX; i++)
        if (!same_real(&a->stack[i], &b->stack[i]) || a->unit[i] != b->unit[i]) return false;
    for (int i = 0; i < REG_BASE_COUNT; i++)
        if (!same_real(&a->regs[i], &b->regs[i])) return false;
    for (int i = 0; i < REG_EXTRA_MAX; i++)
        if (!same_real(&a->extra[i], &b->extra[i])) return false;
    for (int i = 0; i < SUM_COUNT; i++)
        if (!same_real(&a->stats.sum[i], &b->stats.sum[i])) return false;
    return true;
}

static uint8_t argument(calc_cmd_t cmd, int variant)
{
    switch (cmd) {
        case CMD_DIGIT: return (uint8_t)(variant % 10);
        case CMD_RECORD_TAG: return (uint8_t)(1 + variant % 7);
        case CMD_ALPHA_LETTER: return (uint8_t)(variant % 26);
        case CMD_DATE_FN: return (uint8_t)(variant % (DF_FORMAT_STRING + 1));
        case CMD_BASE_OP: return (uint8_t)(variant % (BOP_DIV2 + 1));
        case CMD_BASE_RADIX: { const uint8_t r[] = { 2, 8, 10, 16 }; return r[variant % 4]; }
        case CMD_UNIT: return (uint8_t)(variant % UNIT_COUNT);
        case CMD_STAT_REG: return (uint8_t)(variant % SUM_COUNT);
        case CMD_FIT_MODEL: return (uint8_t)(variant % FIT_COUNT);
        case CMD_TVM_VAR: return (uint8_t)(variant % 7 | (variant % 2 ? TVM_RECALL : 0));
        case CMD_TVM_IYR1: case CMD_TVM_IYR2: case CMD_TVM_ITER_MAX:
            return variant % 2 ? TVM_RECALL : 0;
        case CMD_STO: case CMD_STO_ADD: case CMD_STO_SUB: case CMD_STO_MUL: case CMD_STO_DIV:
        case CMD_RCL: case CMD_RCL_ADD: case CMD_RCL_SUB: case CMD_RCL_MUL: case CMD_RCL_DIV:
        case CMD_XCH: { const uint8_t r[] = { 0, 1, 27, 28, 41, 43 }; return r[variant % 6]; }
        default: return (uint8_t)(variant % 20);
    }
}

int main(void)
{
    unsigned checked = 0;
    const double values[] = { -10, -1, 0, 0.25, 1, 2, 3.5, 7, 42, 1000000 };
    for (int command = CMD_NONE; command <= CMD_GET_STACK_SIZE; command++) {
        if (command == CMD_SEED) continue; /* mock real_t has host-only padding */
        int variants = command == CMD_DATE_FN ? DF_FORMAT_STRING + 1 :
                       command == CMD_BASE_OP ? BOP_DIV2 + 1 : 6;
        for (int variant = 0; variant < variants; variant++)
            for (int scenario = 0; scenario < 24; scenario++) {
                calc_t old, current;
                calc_init(&current);
                current.depth = scenario % 2 ? STACK_MIN : STACK_MAX;
                current.lift = scenario % 3 != 0;
                current.degrees = scenario % 2;
                current.extra_count = 3;
                current.lastx = mk(99);
                current.unit_lastx = 7;
                for (int i = 0; i < STACK_MAX; i++) {
                    current.stack[i] = mk(values[(scenario + i) % 10]);
                    current.unit[i] = (uint8_t)(i % 3);
                }
                if (scenario >= 20) {
                    dt_obj_t obj = { .type = OBJ_DATE, .year = 2024, .month = 3, .day = 14 };
                    current.stack[scenario % 2] = obj_pack(&obj);
                }
                for (int i = 0; i < REG_BASE_COUNT; i++) current.regs[i] = mk(i + 1);
                current.regs[REG_I - 1] = mk(REG_A);
                if (command == CMD_PERM || command == CMD_COMB || command == CMD_LCM) {
                    current.stack[0] = mk(2);
                    current.stack[1] = mk(7);
                }
                for (int i = 1; i <= 4; i++) {
                    real_t x = mk(i), y = mk(2 * i + 3);
                    stats_add(&current.stats, &x, &y, 1);
                }
                if (scenario % 4 == 0) current.last_cmd = CMD_TVM_VAR;
                if (scenario % 5 == 0) current.base.radix = 16;
                old = current;
                original_calc_init(&old); /* initialize original's private constants */
                old = current;
                real_t fin_before[7], fin_after[7];
                const double fin[] = { 12, 6, -1000, 100, 0, 12, 12 };
                for (int i = 0; i < 7; i++) fin_before[i] = mk(fin[i]);
                const int clock_before[] = { 2026, 10, 18, 9, 30, 0 };
                memcpy(mock_fin, fin_before, sizeof mock_fin);
                memcpy(mock_clock, clock_before, sizeof mock_clock);
                calc_seed_random(123);
                uint8_t arg = argument((calc_cmd_t)command, variant);
                original_calc_execute(&old, (calc_cmd_t)command, arg);
                memcpy(fin_after, mock_fin, sizeof fin_after);
                int clock_after[6]; memcpy(clock_after, mock_clock, sizeof mock_clock);
                memcpy(mock_fin, fin_before, sizeof mock_fin);
                memcpy(mock_clock, clock_before, sizeof mock_clock);
                calc_seed_random(123);
                calc_execute(&current, (calc_cmd_t)command, arg);
                bool equal = same_state(&old, &current) && memcmp(clock_after, mock_clock, sizeof mock_clock) == 0;
                for (int i = 0; i < 7; i++) equal = equal && same_real(&fin_after[i], &mock_fin[i]);
                if (!equal) {
                    fprintf(stderr, "Mismatch: command=%d argument=%u scenario=%d\n", command, arg, scenario);
                    return 1;
                }
                checked++;
            }
    }
    printf("%u original/refactored command-state comparisons matched\n", checked);
    return 0;
}
