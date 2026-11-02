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
static appvar_t vars[3];               /* RPNCEdat, RPNCEcfg, RPNCEunt */
static bool open_fails;
static int finishes, fails;

uint8_t ti_Open(const char *name, const char *mode)
{
    int i = strcmp(name, "RPNCEdat") == 0 ? 0 : strcmp(name, "RPNCEcfg") == 0 ? 1 : 2;
    assert(i != 2 || strcmp(name, "RPNCEunt") == 0);
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
int ti_Delete(const char *name)
{
    assert(strcmp(name, "RPNCEunt") == 0);
    vars[2].exists = false;
    vars[2].size = 0;
    return 1;
}

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


/* Historical on-disk schema, frozen here to exercise the actual importer. */
typedef struct {
    real_t sum[SUM_COUNT]; bool linear_only, lnx_ok, lny_ok; uint8_t model;
} old_stats_t;
typedef struct {
    uint8_t magic, version, depth;
    real_t stack[STACK_MAX]; uint8_t unit[STACK_MAX];
    real_t lastx; uint8_t unit_lastx;
    real_t regs[REG_BASE_COUNT];
    uint8_t extra_count, lift, degrees, disp_style; int8_t disp_digits;
    tvm_settings_t tvm; old_stats_t stats; base_state_t base;
    real_t extra[REG_EXTRA_MAX];
} old_state_t;

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
        !memcmp(&a->stats, &b->stats, sizeof(old_stats_t)) &&
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
static void old_fixture(bool footer, bool sidecar)
{
    old_state_t old;
    memset(&old, 0, sizeof old);
    old.magic = 0x52; old.version = 5; old.depth = 4; old.extra_count = 2;
    old.lift = old.degrees = 1; old.disp_style = DISP_FIX; old.disp_digits = DISP_FLOAT;
    for (int i = 0; i < STACK_MAX; i++) old.stack[i] = mk(i);
    for (int i = 0; i < REG_BASE_COUNT; i++) old.regs[i] = mk(i + 10);
    old.lastx = mk(-42); old.unit[0] = 1; old.unit_lastx = 2;
    /* A genuine packed DATE record from version 5. */
    old.stack[1].sign = 0x61;
    old.stack[1].exp = (int8_t)(2026 & 255);
    old.stack[1].mant[0] = 2026 >> 8;
    old.stack[1].mant[1] = 10; old.stack[1].mant[2] = 18;
    old.extra[0] = mk(100); old.extra[1] = old.stack[1];
    tvm_settings_reset(&old.tvm); old.base.radix = 10; old.base.bits = 32;
    vars[0].exists = true;
    vars[0].size = offsetof(old_state_t, extra) + sizeof(real_t) * old.extra_count;
    memcpy(vars[0].data, &old, vars[0].size);
    size_t count = REG_BASE_COUNT + old.extra_count;
    if (footer) {
        unsigned char *tail = vars[0].data + vars[0].size;
        tail[0] = 'U'; tail[1] = 1;
        memset(tail + 2, 0, count); tail[2] = 3;
        vars[0].size += 2 + count;
    }
    vars[2].exists = sidecar;
    if (sidecar) {
        unsigned char *u = vars[2].data;
        u[0] = 'U'; u[1] = 'N'; u[2] = 1; u[3] = count;
        uint32_t h = 2166136261UL;
        const unsigned char *p = (const unsigned char *)old.regs;
        for (size_t i = 0; i < sizeof old.regs; i++) { h ^= p[i]; h *= 16777619UL; }
        h ^= old.extra_count; h *= 16777619UL;
        p = (const unsigned char *)old.extra;
        for (size_t i = 0; i < sizeof(real_t) * old.extra_count; i++) { h ^= p[i]; h *= 16777619UL; }
        for (int i = 0; i < 4; i++) u[4 + i] = h >> (8 * i);
        memset(u + 8, 0, count); u[8] = 4;
        h = 2166136261UL;
        for (size_t i = 0; i < 8 + count; i++) { h ^= u[i]; h *= 16777619UL; }
        for (int i = 0; i < 4; i++) u[8 + count + i] = h >> (8 * i);
        vars[2].size = 12 + count;
    }
}

static void old_precision_fixture(void)
{
    old_state_t old;
    memcpy(&old, vars[0].data, offsetof(old_state_t, extra));
    unsigned char *p = vars[0].data + vars[0].size;
    const real_t first[2] = { mk(7), mk(11) };
    const size_t size = 2 + sizeof first + 1 + 4;
    p[0] = 'P'; p[1] = 1;
    memcpy(p + 2, first, sizeof first); p[2 + sizeof first] = 3;
    uint32_t h = 2166136261UL;
    const unsigned char *s = (const unsigned char *)&old.stats;
    for (size_t i = 0; i < sizeof old.stats; i++) { h ^= s[i]; h *= 16777619UL; }
    for (size_t i = 0; i < size - 4; i++) { h ^= p[i]; h *= 16777619UL; }
    for (int i = 0; i < 4; i++) p[size - 4 + i] = h >> (8 * i);
    vars[0].size += size;
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
        vars[2].exists = true;
        storage_save(&original);
        if (!i) fixed_size = vars[0].size;
        check("save finalizes entry and archives current state", finishes == before + 1 && vars[0].archived);
        check("all configured objects survive save/load", storage_load(&loaded) && same_saved(&original, &loaded));
        check("unused extra registers are omitted", vars[0].size == fixed_size + sizeof(value_t) * counts[i]);
        check("current saves remove the retired unit sidecar", !vars[2].exists);
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
    for (int sidecar = 0; sidecar <= 1; sidecar++) {
        old_fixture(true, sidecar);
        old_precision_fixture();
        memset(&loaded, 0, sizeof loaded);
        check("version 5 imports real values, DATEs, units and LASTX", storage_load(&loaded) &&
            loaded.stack[0].real.v == 0 && loaded.stack[0].unit == 1 &&
            value_is_date(&loaded.stack[1]) && loaded.stack[1].date.year == 2026 &&
            loaded.lastx.real.v == -42 && loaded.lastx.unit == 2 &&
            loaded.regs[0].unit == (sidecar ? 4 : 3) &&
            value_is_date(&loaded.extra[1]) && !loaded.complex_results && loaded.complex_mode == COMPLEX_RECT);
        check("version 5 precision footer survives workspace reuse", loaded.stats.constant == 3 &&
              loaded.stats.first[0].v == 7 && loaded.stats.first[1].v == 11);
        original = loaded;
        storage_save(&loaded);
        check("imported state writes only version 6 and reopens", vars[0].data[1] == 6 &&
            !vars[2].exists && storage_load(&loaded) && same_saved(&original, &loaded));
    }
    old_fixture(true, true);
    vars[2].data[10] ^= 1;
    check("damaged old sidecar drops only register units", storage_load(&loaded) &&
          !loaded.regs[0].unit && value_is_date(&loaded.stack[1]) && loaded.lastx.unit == 2);
    old_fixture(false, false); vars[0].size--;
    rejected_unchanged("truncated version 5 retains runtime state", &loaded);
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
    /* A genuine version-2 file has exactly the old prefix, no new byte. */
    size_t settings_size = vars[1].size;
    vars[1].data[1] = 2;
    vars[1].size = settings_size - 1;
    memset(&loaded, 0, sizeof loaded);
    loaded.decimal_comma = true;
    check("version-2 settings retain DATE/time zones and default to decimal point",
          storage_load_settings(&loaded) == 3 && !loaded.decimal_comma &&
          loaded.date_as_string && loaded.app_tz == -28 && loaded.clock_tz == 4);
    vars[1].data[1] = 3;
    loaded.decimal_comma = true;
    check("truncated new settings default to decimal point",
          storage_load_settings(&loaded) == 3 && !loaded.decimal_comma && loaded.app_tz == -28);
    vars[1].size = settings_size;
    vars[1].data[settings_size - 1] = 2;
    loaded.decimal_comma = true;
    check("invalid separator preference falls back to decimal point",
          storage_load_settings(&loaded) == 3 && !loaded.decimal_comma);
    vars[1].data[1] = 1;
    vars[1].size = 3;
    memset(&loaded, 0, sizeof loaded);
    loaded.app_tz = 12;
    check("tint-only settings retain DATE defaults",
          storage_load_settings(&loaded) == 3 && loaded.app_tz == 12 && !loaded.date_as_string);
    printf("%d failures\n", fails);
    return fails != 0;
}
