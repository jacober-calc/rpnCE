/* Screenshot engine driver: runs RPNCE's real main loop (src/main.c, built
 * with main renamed to rpnce_main), display and core on a PC, with keys
 * from a script and frames written as PPM images.
 *
 *   driver SCRIPT OUTDIR
 *
 * SCRIPT is whitespace-separated tokens:
 *   sk_Enter, sk_2nd, ...   press a key (key codes from ti/getcsc.h)
 *   ON                      press the ON key
 *   SNAP name               write the frame on screen to OUTDIR/name.ppm
 *   FLASH name              write the frame shown during the last delay
 *                           (a completed STO/RCL/FIX prompt) to OUTDIR/name.ppm
 * The driver stops at the end of the script.
 *
 * Numbers use the PC stand-ins in tests/mock (doubles), rounded to the
 * calculator's 14 significant digits and formatted by RPNCE's real
 * format_digits, so the display shows RPNCE's actual formatting. */
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/real.h>
#include <ti/error.h>
#include <ti/getcsc.h>
#include "graphx.h"
#include "calc.h"
#include "format.h"
#include "../../src/sys/storage.h"

int rpnce_main(void);

/* ---------- calculator routines (as tests/mock_calc_runtime.c) ---------- */

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
real_t round_decimals(const real_t *v, int n) { double p = pow(10, n); return mk(round(v->v * p) / p); }
real_t round_significant(const real_t *v, int n)
{
    if (v->v == 0) return *v;
    int e = (int)floor(log10(fabs(v->v)));
    return round_decimals(v, n - 1 - e);
}

/* The calculator keeps 14 significant digits; format them with the real
 * formatter. */
void format_real(char *out, const real_t *v, uint8_t style, int8_t digits,
                 int max_digits, int max_exp_digits, bool group)
{
    uint8_t d[14] = { 0 };
    int exp = 0;
    double x = fabs(v->v);
    if (x != 0) {
        char s[40];
        snprintf(s, sizeof s, "%.13e", x);      /* d.ddddddddddddde+XX */
        d[0] = (uint8_t)(s[0] - '0');
        for (int i = 0; i < 13; i++)
            d[i + 1] = (uint8_t)(s[2 + i] - '0');
        exp = atoi(strchr(s, 'e') + 1);
    }
    format_digits(out, v->v < 0, exp, d, style, digits, max_digits, max_exp_digits, group);
}

/* a fixed clock: the guide's edition date */
static int clock_now[6] = { 2026, 10, 19, 9, 30, 0 };
bool sys_clock_read(int *y, int *mo, int *d, int *h, int *mi, int *s)
{ *y = clock_now[0]; *mo = clock_now[1]; *d = clock_now[2]; *h = clock_now[3]; *mi = clock_now[4]; *s = clock_now[5]; return true; }
void sys_clock_write(int y, int mo, int d, int h, int mi, int s)
{ clock_now[0] = y; clock_now[1] = mo; clock_now[2] = d; clock_now[3] = h; clock_now[4] = mi; clock_now[5] = s; }

/* nothing is saved or restored: every script starts fresh */
void storage_save(calc_t *c) { (void)c; }
bool storage_load(calc_t *c) { (void)c; return false; }
void storage_save_settings(uint8_t tint, const calc_t *c) { (void)tint; (void)c; }
uint8_t storage_load_settings(calc_t *c) { (void)c; return 0; }

/* the RPNCEres appvar, from data/RPNCEres.bin */
uint8_t mock_res[65536];
size_t mock_res_size;

/* ---------- script ---------- */

bool shot_on_key;
static char **tok;
static int ntok, pos;
static const char *outdir;
static uint8_t flash_frame[320 * 240];
static bool have_flash;

static void write_ppm(const char *name, const uint8_t *frame)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6 320 240 255\n");
    for (int i = 0; i < 320 * 240; i++) {
        uint16_t c = gfx_palette[frame[i]];
        uint8_t r = (c >> 10) & 31, gg = (c >> 5) & 31, b = c & 31;
        uint8_t px[3] = { (uint8_t)(r << 3 | r >> 2), (uint8_t)(gg << 3 | gg >> 2), (uint8_t)(b << 3 | b >> 2) };
        fwrite(px, 1, 3, f);
    }
    fclose(f);
}

void delay(uint16_t ms)
{
    (void)ms;
    memcpy(flash_frame, shot_front(), sizeof flash_frame);
    have_flash = true;
}

static const struct { const char *name; sk_key_t code; } keymap[] = {
#define K(x) { #x, x },
    K(sk_Down) K(sk_Left) K(sk_Right) K(sk_Up) K(sk_Enter) K(sk_Add) K(sk_Sub) K(sk_Mul)
    K(sk_Div) K(sk_Power) K(sk_Clear) K(sk_Chs) K(sk_3) K(sk_6) K(sk_9) K(sk_RParen)
    K(sk_Tan) K(sk_Vars) K(sk_DecPnt) K(sk_2) K(sk_5) K(sk_8) K(sk_LParen) K(sk_Cos)
    K(sk_Prgm) K(sk_Stat) K(sk_0) K(sk_1) K(sk_4) K(sk_7) K(sk_Comma) K(sk_Sin)
    K(sk_Apps) K(sk_GraphVar) K(sk_Store) K(sk_Ln) K(sk_Log) K(sk_Square) K(sk_Recip)
    K(sk_Math) K(sk_Alpha) K(sk_Graph) K(sk_Trace) K(sk_Zoom) K(sk_Window) K(sk_Yequ)
    K(sk_2nd) K(sk_Mode) K(sk_Del)
#undef K
};

sk_key_t os_GetCSC(void)
{
    shot_on_key = false;                    /* ON is a single press */
    while (pos < ntok) {
        const char *t = tok[pos++];
        if (!strcmp(t, "SNAP") && pos < ntok) {
            write_ppm(tok[pos++], shot_front());
        } else if (!strcmp(t, "FLASH") && pos < ntok) {
            if (!have_flash) { fprintf(stderr, "FLASH without a prompt flash\n"); exit(1); }
            write_ppm(tok[pos++], flash_frame);
        } else if (!strcmp(t, "ON")) {
            shot_on_key = true;
            return 0;
        } else {
            for (unsigned i = 0; i < sizeof keymap / sizeof *keymap; i++)
                if (!strcmp(t, keymap[i].name)) {
                    have_flash = false;
                    return keymap[i].code;
                }
            fprintf(stderr, "unknown token %s\n", t);
            exit(1);
        }
    }
    /* the end of the script: nothing is saved here, so just stop */
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: driver SCRIPT OUTDIR\n"); return 2; }
    outdir = argv[2];
    FILE *f = fopen("data/RPNCEres.bin", "rb");
    if (!f) { perror("data/RPNCEres.bin"); return 1; }
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (getenv("SHOT_NO_RES"))
        mock_res_size = 0;              /* RPNCEres missing: the startup notice */

    FILE *s = fopen(argv[1], "r");
    if (!s) { perror(argv[1]); return 1; }
    static char text[1 << 16];
    size_t n = fread(text, 1, sizeof text - 1, s);
    fclose(s);
    text[n] = '\0';
    static char *list[8192];
    for (char *p = strtok(text, " \t\r\n"); p && ntok < 8192; p = strtok(NULL, " \t\r\n"))
        list[ntok++] = p;
    tok = list;

    /* TI-OS's finance variables start with P/Y and C/Y at 12, the rest 0 */
    for (int i = 0; i < 7; i++)
        mock_fin[i] = mk(0);
    mock_fin[5] = mk(12);
    mock_fin[6] = mk(12);
    return rpnce_main();
}
