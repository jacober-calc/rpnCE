/* Exercises the actual storage reader/writer with in-memory appvars.
 * Core validation uses the real BASE, TVM and DATE modules; only the OS
 * number routines and the entry-finalization boundary are stand-ins. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ti/error.h>
#include <fileioc.h>
#include "../src/sys/storage.h"
#include "format.h"
#include "units.h"
#include "dateobj.h"

typedef struct {
    unsigned char data[16384];
    size_t size, pos;
    bool exists, archived;
} appvar_t;
static appvar_t vars[2];
static bool open_fails;
static int finishes, fails;

uint8_t ti_Open(const char *name, const char *mode)
{
    int i = strcmp(name, "RPNCEdat") == 0 ? 0 : 1;
    assert(i == 0 || strcmp(name, "RPNCEcfg") == 0);
    if (open_fails || (*mode == 'r' && !vars[i].exists)) return 0;
    if (*mode == 'w') {
        vars[i].size = 0;
        vars[i].exists = true;
        vars[i].archived = false;
    }
    vars[i].pos = 0;
    return (uint8_t)(i + 1);
}
size_t ti_Read(void *data, size_t size, size_t count, uint8_t handle)
{
    appvar_t *v = &vars[handle - 1];
    if (count > (v->size - v->pos) / size) count = (v->size - v->pos) / size;
    memcpy(data, v->data + v->pos, size * count);
    v->pos += size * count;
    return count;
}
size_t ti_Write(const void *data, size_t size, size_t count, uint8_t handle)
{
    appvar_t *v = &vars[handle - 1];
    assert(v->pos + size * count <= sizeof v->data);
    memcpy(v->data + v->pos, data, size * count);
    v->pos += size * count;
    if (v->pos > v->size) v->size = v->pos;
    return count;
}
int ti_Close(uint8_t handle) { (void)handle; return 1; }
bool ti_SetArchiveStatus(bool archived, uint8_t handle)
{ vars[handle - 1].archived = archived; return true; }

/* Entry parsing is tested in test_calc.c; storage must still call it. */
void calc_finish_entry(calc_t *c) { assert(!c->editing); finishes++; }
real_t mock_fin[7];
jmp_buf mock_jmp;
real_t os_RealDiv(const real_t *a, const real_t *b)
{ if (b->v == 0) os_ThrowError(OS_E_DIVBY0); return mk(a->v / b->v); }
real_t os_RealLog(const real_t *a)
{ if (a->v <= 0) os_ThrowError(OS_E_DOMAIN); return mk(log(a->v)); }
real_t round_decimals(const real_t *v, int n)
{ double p = pow(10, n); return mk(round(v->v * p) / p); }

static void check(const char *name, bool pass)
{
    if (!pass) fails++;
    printf("%s %s\n", pass ? "ok  " : "FAIL", name);
}

static bool same_real(const real_t *a, const real_t *b)
{
    return a->sign == b->sign && a->exp == b->exp &&
           memcmp(a->mant, b->mant, sizeof a->mant) == 0 &&
           (obj_is(a) || a->v == b->v);
}

static void populate(calc_t *c, int count)
{
    memset(c, 0, sizeof *c);
    c->depth = STACK_MAX;
    c->extra_count = (uint8_t)count;
    for (int i = 0; i < STACK_MAX; i++) {
        c->stack[i] = mk(i + 0.125);
        c->unit[i] = (uint8_t)(i + 1);
    }
    dt_obj_t date = { .type = OBJ_DATE, .year = 2026, .month = 10, .day = 18 };
    c->stack[2] = obj_pack(&date);
    c->lastx = mk(-42.5);
    c->unit_lastx = UNIT_COUNT;
    for (int i = 0; i < REG_BASE_COUNT; i++) {
        c->regs[i] = mk(-i - 0.5);
        c->reg_unit[i] = (uint8_t)(i + 1);
    }
    for (int i = 0; i < count; i++) {
        c->extra[i] = mk(i + 1000);
        c->extra_unit[i] = (uint8_t)(i + 1);
    }
    c->lift = true;
    c->degrees = true;
    c->disp_style = DISP_ENG;
    c->disp_digits = 8;
    c->tvm.begin = true;
    tvm_settings_reset(&c->tvm);
    c->tvm.iyr1 = mk(3.25);
    c->tvm.iyr1_set = true;
    c->stats.model = FIT_PWR;
    c->stats.lnx_ok = true;
    c->stats.linear_only = true;
    for (int i = 0; i < SUM_COUNT; i++) c->stats.sum[i] = mk(i + 2000);
    c->base.radix = 16;
    c->base.bits = BASE_MAX_BITS;
    c->base.carry = true;
    c->date_as_string = true;
    c->epoch = EPOCH_CUSTOM;
    c->custom_epoch = date;
    c->app_tz = -28;
    c->clock_tz = 4;
}

static bool same_saved(const calc_t *a, const calc_t *b)
{
    if (a->depth != b->depth || a->extra_count != b->extra_count ||
        a->lift != b->lift || a->degrees != b->degrees ||
        a->disp_style != b->disp_style || a->disp_digits != b->disp_digits ||
        a->unit_lastx != b->unit_lastx || !same_real(&a->lastx, &b->lastx) ||
        a->base.radix != b->base.radix || a->base.bits != b->base.bits ||
        a->base.carry != b->base.carry || a->stats.model != b->stats.model ||
        a->stats.linear_only != b->stats.linear_only || a->stats.lnx_ok != b->stats.lnx_ok ||
        a->stats.lny_ok != b->stats.lny_ok || a->tvm.begin != b->tvm.begin ||
        a->tvm.iter_max != b->tvm.iter_max || a->tvm.iyr1_set != b->tvm.iyr1_set ||
        a->tvm.iyr2_set != b->tvm.iyr2_set || a->tvm.iter_max_set != b->tvm.iter_max_set ||
        !same_real(&a->tvm.iyr1, &b->tvm.iyr1) || !same_real(&a->tvm.iyr2, &b->tvm.iyr2))
        return false;
    for (int i = 0; i < STACK_MAX; i++)
        if (!same_real(&a->stack[i], &b->stack[i]) || a->unit[i] != b->unit[i]) return false;
    for (int i = 0; i < REG_BASE_COUNT; i++)
        if (!same_real(&a->regs[i], &b->regs[i]) || a->reg_unit[i] != b->reg_unit[i]) return false;
    for (int i = 0; i < a->extra_count; i++)
        if (!same_real(&a->extra[i], &b->extra[i]) || a->extra_unit[i] != b->extra_unit[i]) return false;
    for (int i = 0; i < SUM_COUNT; i++)
        if (!same_real(&a->stats.sum[i], &b->stats.sum[i])) return false;
    return true;
}

static void rejected_unchanged(const char *name, calc_t *c)
{
    calc_t before;
    memcpy(&before, c, sizeof before);
    bool loaded = storage_load(c);
    check(name, !loaded && memcmp(c, &before, sizeof before) == 0);
}

int main(void)
{
    calc_t original, loaded;
    populate(&original, 0);
    loaded = original;
    rejected_unchanged("missing state leaves the calculator unchanged", &loaded);

    size_t fixed_size = 0;
    const int counts[] = { 0, 5, REG_EXTRA_MAX, 0, REG_EXTRA_MAX };
    for (unsigned n = 0; n < sizeof counts / sizeof counts[0]; n++) {
        populate(&original, counts[n]);
        int previous_finishes = finishes;
        storage_save(&original);
        check("save finalizes entry and archives the appvar", finishes == previous_finishes + 1 && vars[0].archived);
        if (n == 0) fixed_size = vars[0].size;
        check("save writes only the configured extra registers",
              vars[0].size == fixed_size + (sizeof(real_t) + 1) * counts[n]);
        memset(&loaded, 0, sizeof loaded);
        check("all persistent state survives repeated save/load",
              storage_load(&loaded) && same_saved(&original, &loaded));
    }

    unsigned char saved[16384];
    size_t saved_size = vars[0].size;
    memcpy(saved, vars[0].data, saved_size);
    const int bad_offsets[] = { 0, 1, 2 };
    for (unsigned i = 0; i < sizeof bad_offsets / sizeof bad_offsets[0]; i++) {
        vars[0].data[bad_offsets[i]] = 0;
        rejected_unchanged("bad magic, version or depth is rejected", &loaded);
        memcpy(vars[0].data, saved, saved_size);
    }
    vars[0].size = fixed_size - 1;
    rejected_unchanged("truncated fixed state is rejected", &loaded);
    vars[0].size = saved_size - 1;
    rejected_unchanged("truncated extra registers are rejected", &loaded);
    vars[0].size = saved_size;
    check("valid load after failed loads recovers the saved state",
          storage_load(&loaded) && same_saved(&original, &loaded));

    size_t legacy_size = saved_size - 2 - REG_BASE_COUNT - original.extra_count;
    vars[0].size = legacy_size + 1;
    rejected_unchanged("truncated extension header is rejected", &loaded);
    vars[0].size = saved_size;
    vars[0].data[legacy_size] = '?';
    rejected_unchanged("unknown unit extension is rejected", &loaded);
    memcpy(vars[0].data, saved, saved_size);
    vars[0].data[saved_size - 1] = 255;
    check("invalid saved unit tags become scalar values",
          storage_load(&loaded) && loaded.extra_unit[original.extra_count - 1] == 0 &&
          same_real(&loaded.extra[original.extra_count - 1], &original.extra[original.extra_count - 1]));
    memcpy(vars[0].data, saved, saved_size);

    /* Legacy v5 ends before the optional unit extension. Preserve its values
     * and stack units while initializing the new register metadata. */
    vars[0].size = saved_size - 2 - REG_BASE_COUNT - original.extra_count;
    memset(original.reg_unit, 0, sizeof original.reg_unit);
    memset(original.extra_unit, 0, sizeof original.extra_unit);
    check("legacy saves load without losing numeric or DATE values",
          storage_load(&loaded) && same_saved(&original, &loaded));
    storage_save(&loaded);
    check("legacy state gains unit extension and reloads",
          vars[0].data[1] == 5 && vars[0].size == saved_size && storage_load(&loaded) && same_saved(&original, &loaded));

    /* Loading and then saving a smaller state must not retain old registers. */
    populate(&original, 0);
    storage_save(&original);
    check("shrinking after load drops the unused saved tail", vars[0].size == fixed_size);
    open_fails = true;
    rejected_unchanged("open failure leaves state unchanged", &loaded);
    storage_save(&original);
    check("failed save leaves the previous appvar intact", vars[0].size == fixed_size);
    open_fails = false;

    storage_save_settings(3, &original);
    memset(&loaded, 0, sizeof loaded);
    check("settings preserve tint, DATE format, epoch and time zones",
          vars[1].archived && storage_load_settings(&loaded) == 3 && loaded.date_as_string &&
          loaded.epoch == EPOCH_CUSTOM && loaded.custom_epoch.year == 2026 &&
          loaded.custom_epoch.month == 10 && loaded.custom_epoch.day == 18 &&
          loaded.app_tz == -28 && loaded.clock_tz == 4);
    vars[1].data[1] = 1;
    vars[1].size = 3;
    memset(&loaded, 0, sizeof loaded);
    loaded.app_tz = 12;
    check("legacy tint-only settings retain DATE defaults",
          storage_load_settings(&loaded) == 3 && loaded.app_tz == 12 && !loaded.date_as_string);
    printf("%d failures\n", fails);
    return fails != 0;
}
