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
#include "value.h"
#include <stddef.h>

typedef struct {
    unsigned char data[16384];
    size_t size, pos;
    bool exists, archived;
} appvar_t;
static appvar_t vars[2];               /* RPNCEdat, RPNCEcfg */
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


static bool same_saved(const calc_t *a, const calc_t *b)
{
    return a->depth == b->depth && a->extra_count == b->extra_count &&
        a->lift == b->lift && a->degrees == b->degrees &&
        a->complex_results == b->complex_results && a->complex_mode == b->complex_mode &&
        a->disp_style == b->disp_style && a->disp_digits == b->disp_digits &&
        !memcmp(a->stack, b->stack, sizeof a->stack) &&
        !memcmp(&a->lastx, &b->lastx, sizeof a->lastx) &&
        !memcmp(a->regs, b->regs, sizeof a->regs) &&
        !memcmp(a->extra, b->extra, sizeof(value_t) * a->extra_count) &&
        !memcmp(&a->stats, &b->stats, offsetof(stats_t, first)) &&
        !memcmp(&a->tvm, &b->tvm, sizeof a->tvm) &&
        !memcmp(&a->base, &b->base, sizeof a->base);
}
static void populate(calc_t *c, int count)
{
    memset(c, 0, sizeof *c);
    c->depth = STACK_MAX; c->extra_count = count;
    for (int i = 0; i < STACK_MAX; i++) {
        c->stack[i] = value_real(mk(i + .125)); c->stack[i].unit = i + 1;
    }
    dt_obj_t date = { .type = OBJ_DATE, .year = 2026, .month = 10, .day = 18 };
    c->stack[2] = value_date(&date);
    c->stack[3] = value_complex(mk(-2), mk(5));
    c->lastx = value_complex(mk(1), mk(-7));
    for (int i = 0; i < REG_BASE_COUNT; i++) {
        c->regs[i] = value_real(mk(-i - .5)); c->regs[i].unit = i + 1;
    }
    c->regs[3] = value_complex(mk(3), mk(-4));
    c->regs[4] = value_date(&date);
    for (int i = 0; i < count; i++) {
        c->extra[i] = value_complex(mk(i), mk(-i));
    }
    c->lift = c->degrees = c->complex_results = true;
    c->complex_mode = COMPLEX_DEG;
    c->disp_style = DISP_ENG; c->disp_digits = 8;
    tvm_settings_reset(&c->tvm); c->tvm.begin = true;
    c->stats.model = FIT_PWR; c->stats.lnx_ok = true;
    for (int i = 0; i < SUM_COUNT; i++) c->stats.sum[i] = mk(i + 2000);
    c->base.radix = 16; c->base.bits = BASE_MAX_BITS; c->base.carry = true;
    c->date_as_string = true; c->epoch = EPOCH_CUSTOM; c->custom_epoch = date;
    c->app_tz = -28; c->clock_tz = 4;
}
static void rejected_unchanged(const char *name, calc_t *c)
{
    calc_t before = *c;
    check(name, !storage_load(c) && !memcmp(c, &before, sizeof before));
}
int main(void)
{
    calc_t original, loaded;
    populate(&original, 0); loaded = original;
    rejected_unchanged("missing state leaves calculator unchanged", &loaded);
    size_t fixed_size = 0;
    const int counts[] = { 0, 5, REG_EXTRA_MAX, 0, REG_EXTRA_MAX };
    for (unsigned i = 0; i < sizeof counts / sizeof *counts; i++) {
        populate(&original, counts[i]);
        original.degrees = (i & 1) != 0;
        loaded.degrees = !original.degrees;
        int before = finishes;
        storage_save(&original);
        if (!i) fixed_size = vars[0].size;
        check("save finalizes entry and archives current state", finishes == before + 1 && vars[0].archived);
        check("all configured objects survive save/load", storage_load(&loaded) && same_saved(&original, &loaded));
        check("unused extra registers are omitted", vars[0].size == fixed_size + sizeof(value_t) * counts[i]);
    }
    unsigned char saved[16384];
    size_t saved_size = vars[0].size;
    memcpy(saved, vars[0].data, saved_size);
    for (int i = 0; i < 3; i++) {
        vars[0].data[i] = 0;
        rejected_unchanged("invalid header rejected without mutation", &loaded);
        memcpy(vars[0].data, saved, saved_size);
    }
    const size_t precision_size = 2 + 2 * sizeof(real_t) + 1 + 4;
    vars[0].size = fixed_size - precision_size - 1;
    rejected_unchanged("truncated fixed objects rejected", &loaded);
    vars[0].size = saved_size - precision_size - 1;
    rejected_unchanged("truncated extra objects rejected", &loaded);
    vars[0].size = saved_size;
    check("valid state still loads after rejection", storage_load(&loaded) && same_saved(&original, &loaded));
    /* Corrupt a value's discriminator, not its numeric payload. */
    value_t fixture = original.stack[0];
    size_t object_offset = 0;
    for (size_t i = 3; i + sizeof fixture <= saved_size; i++)
        if (!memcmp(saved + i, &fixture, sizeof fixture)) { object_offset = i; break; }
    check("fixture locates a complete saved value", object_offset != 0);
    vars[0].data[object_offset + offsetof(value_t, type)] = 255;
    rejected_unchanged("unknown object type rejected without mutation", &loaded);
    memcpy(vars[0].data, saved, saved_size);
    original.stats.first[0] = mk(7); original.stats.first[1] = mk(11); original.stats.constant = 3;
    storage_save(&original);
    check("precision witnesses round trip", storage_load(&loaded) && loaded.stats.constant == 3 &&
          loaded.stats.first[0].v == 7 && loaded.stats.first[1].v == 11);
    vars[0].data[vars[0].size - 1] ^= 1;
    check("damaged witnesses retain sums without claiming constancy", storage_load(&loaded) &&
          !loaded.stats.constant && same_saved(&original, &loaded));
    vars[0].size -= precision_size;
    check("absent witnesses retain saved objects", storage_load(&loaded) && !loaded.stats.constant && same_saved(&original, &loaded));
    /* rpnCE 0.7.0 saves (format 5) are not read; they are left alone. */
    memcpy(vars[0].data, saved, saved_size);
    vars[0].size = saved_size;
    vars[0].data[1] = 5;
    rejected_unchanged("format 5 saves are not read", &loaded);
    check("an unread save is not modified", vars[0].data[1] == 5 && vars[0].size == saved_size);
    populate(&original, 0); storage_save(&original);
    open_fails = true;
    rejected_unchanged("open failure retains runtime state", &loaded);
    storage_save(&original);
    check("open failure retains saved file", vars[0].size == fixed_size);
    open_fails = false;
    original.decimal_comma = true;
    storage_save_settings(3, &original);
    memset(&loaded, 0, sizeof loaded);
    check("settings preserve tint, DATE format, epoch and time zones",
          vars[1].archived && storage_load_settings(&loaded) == 3 && loaded.date_as_string &&
          loaded.epoch == EPOCH_CUSTOM && loaded.custom_epoch.year == 2026 &&
          loaded.custom_epoch.month == 10 && loaded.custom_epoch.day == 18 &&
          loaded.app_tz == -28 && loaded.clock_tz == 4 && loaded.decimal_comma);
    size_t settings_size = vars[1].size;
    vars[1].data[settings_size - 1] = 2;
    loaded.decimal_comma = true;
    check("invalid separator preference falls back to decimal point",
          storage_load_settings(&loaded) == 3 && !loaded.decimal_comma);
    calc_t before = loaded;
    vars[1].size = settings_size - 1;
    check("truncated settings are ignored",
          storage_load_settings(&loaded) == 0 && !memcmp(&before, &loaded, sizeof loaded));
    vars[1].size = settings_size;
    vars[1].data[1] = 2;
    check("settings older than rpnCE 0.7.0 are ignored",
          storage_load_settings(&loaded) == 0 && !memcmp(&before, &loaded, sizeof loaded));
    printf("%d failures\n", fails);
    return fails != 0;
}
