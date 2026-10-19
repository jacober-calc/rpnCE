/* Number, BASE and DATE record entry. Calculator command dispatch remains
 * in calc.c; this module owns editing the entry text and finalizing X. */
#include <string.h>
#include "calc_entry.h"
#include "calc_internal.h"
#include "format.h"
#include "mathx.h"
#include "dateobj.h"

/* ---------- number entry ---------- */

static int find_char(const calc_t *c, char ch)
{
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] == ch)
            return i;
    return -1;
}

static void insert_char(calc_t *c, int pos, char ch)
{
    if (c->entry_len >= ENTRY_MAX)
        return;
    memmove(&c->entry[pos + 1], &c->entry[pos], c->entry_len - pos);
    c->entry[pos] = ch;
    c->entry_len++;
    c->entry[c->entry_len] = '\0';
}

static void remove_char(calc_t *c, int pos)
{
    memmove(&c->entry[pos], &c->entry[pos + 1], c->entry_len - pos - 1);
    c->entry_len--;
    c->entry[c->entry_len] = '\0';
}

void calc_entry_append(calc_t *c, char ch)
{
    insert_char(c, c->entry_len, ch);
}

static void begin_entry(calc_t *c)
{
    if (c->editing)
        return;
    /* Lift now so Y shows the old X while typing; X is replaced at the end. */
    if (c->lift)
        calc_stack_push(c, &c->stack[0]);
    c->lift = true;
    c->editing = true;
    c->entry_len = 0;
    c->entry[0] = '\0';
}

/* The typed digits as a word, in the current radix. */
static base_word_t entry_word(const calc_t *c)
{
    base_word_t w = 0;
    for (int i = 0; i < c->entry_len; i++)
        w = w * c->base.radix + (base_word_t)base_digit_value(&c->base, c->entry[i]);
    return w;
}

/* Turns the typed text into the X value and leaves edit mode. */
void calc_finish_entry(calc_t *c)
{
    if (!c->editing)
        return;
    if (dt_text_is_object(c->entry)) {
        dt_obj_t o;
        if (dt_parse(c->entry, &o) == DT_OK)
            c->stack[0] = obj_pack(&o);
        else
            c->stack[0] = mx_zero;           /* only when leaving mid-record */
    } else if (calc_base_mode(c)) {
        c->stack[0] = base_real(entry_word(c));
    } else {
        c->stack[0] = os_StrToReal(c->entry, NULL);
    }
    c->unit[REG_X] = 0;
    c->editing = false;
}

void calc_entry_clear(calc_t *c)
{
    c->editing = false;
    c->entry_len = 0;
    c->entry[0] = '\0';
    c->stack[0] = mx_zero;
    c->unit[REG_X] = 0;
    c->lift = false;
}

static int mantissa_digits(const calc_t *c)
{
    int n = 0;
    for (int i = 0; i < c->entry_len && c->entry[i] != 'E'; i++)
        if (c->entry[i] >= '0' && c->entry[i] <= '9')
            n++;
    return n;
}

/* A digit in HEX, OCT or BIN: only the radix's digits, and only while the
 * number still fits the word size. */
static void entry_base_digit(calc_t *c, uint8_t d)
{
    if (d >= c->base.radix)
        return;
    begin_entry(c);
    base_word_t w = entry_word(c) * c->base.radix + d;   /* at most 48 bits */
    if (w > base_mask(&c->base) || c->entry_len >= ENTRY_MAX)
        return;
    if (c->entry_len == 1 && c->entry[0] == '0')
        c->entry_len = 0;                   /* no leading zeros */
    calc_entry_append(c, (char)(d < 10 ? '0' + d : 'A' + d - 10));
}

static bool entry_has(const calc_t *c, char ch)
{
    return find_char(c, ch) >= 0;
}

/* Typing a DATE record or a compact duration rather than a number. */
bool calc_entry_is_object(const calc_t *c)
{
    return c->editing && dt_text_is_object(c->entry);
}

/* Inside an open record: after the { and before any }. */
bool calc_entry_in_record(const calc_t *c)
{
    return c->editing && entry_has(c, '{') && !entry_has(c, '}');
}

void calc_entry_digit(calc_t *c, uint8_t d)
{
    if (calc_entry_is_object(c)) {
        if (d <= 9 && !entry_has(c, '}'))
            calc_entry_append(c, (char)('0' + d));
        return;
    }
    if (calc_base_mode(c)) {
        entry_base_digit(c, d);
        return;
    }
    if (d > 9)
        return;                             /* A-F only mean something in HEX */
    begin_entry(c);
    int e = find_char(c, 'E');
    if (e >= 0) {
        int exp_digits = c->entry_len - e - 1;
        if (exp_digits > 0 && c->entry[e + 1] == '-')
            exp_digits--;
        if (exp_digits >= 2)
            return;
    } else if (mantissa_digits(c) >= DISPLAY_DIGITS) {   /* 12, as on the HP-32SII */
        return;
    }
    calc_entry_append(c, (char)('0' + d));
}

void calc_entry_dot(calc_t *c)
{
    if (calc_base_mode(c) || calc_entry_is_object(c))
        return;                             /* whole numbers only */
    begin_entry(c);
    if (find_char(c, '.') >= 0 || find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        calc_entry_append(c, '0');
    calc_entry_append(c, '.');
}

void calc_entry_ee(calc_t *c)
{
    /* Inside a record the , key separates fields; elsewhere it's EE. */
    if (calc_entry_in_record(c)) {
        char last = c->entry[c->entry_len - 1];
        if (last != ',' && last != '{' && last != '-')
            calc_entry_append(c, ',');
        return;
    }
    if (calc_base_mode(c) || calc_entry_is_object(c))
        return;
    begin_entry(c);
    if (find_char(c, 'E') >= 0)
        return;
    if (mantissa_digits(c) == 0)
        calc_entry_append(c, '1');
    calc_entry_append(c, 'E');
}

void calc_entry_chs(calc_t *c)
{
    if (calc_entry_is_object(c)) {
        /* the sign of the field being typed, or of a compact duration */
        int pos = 0;
        for (int i = 0; i < c->entry_len; i++)
            if (c->entry[i] == '{' || c->entry[i] == ',')
                pos = i + 1;
        if (entry_has(c, '}'))
            return;
        if (pos < c->entry_len && c->entry[pos] == '-')
            remove_char(c, pos);
        else
            insert_char(c, pos, '-');
        return;
    }
    int e = find_char(c, 'E');
    int pos = (e >= 0) ? e + 1 : 0;
    if (pos < c->entry_len && c->entry[pos] == '-')
        remove_char(c, pos);
    else
        insert_char(c, pos, '-');
}

void calc_entry_backspace(calc_t *c)
{
    if (c->entry_len > 0)
        remove_char(c, c->entry_len - 1);
    if (c->entry_len == 0)
        calc_entry_clear(c);
}

/* 2ND {: starts a record, after an optional type tag. */
void calc_entry_record_open(calc_t *c)
{
    if (!c->editing)
        begin_entry(c);
    if (entry_has(c, '{'))
        return;
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] < 'A' || c->entry[i] > 'Z')
            return;                         /* only after a tag, or nothing */
    calc_entry_append(c, '{');
}

/* The D{} / T{} / ... menu keys: start a record of that type. */
void calc_entry_record_tag(calc_t *c, uint8_t type)
{
    static const char *const tag[OBJ_COUNT] = { "", "D{", "T{", "DT{", "TZ{", "DZ{", "DW{", "DR{" };
    if (type == OBJ_NONE || type >= OBJ_COUNT)
        return;
    if (!c->editing)
        begin_entry(c);
    if (c->entry_len)
        return;
    for (const char *p = tag[type]; *p; p++)
        calc_entry_append(c, *p);
}

/* ALPHA + key: HEX digits A-F in HEX mode; otherwise a type tag letter
 * (before the {) or a compact-duration unit D, H, M, S after digits. */
void calc_entry_alpha(calc_t *c, uint8_t letter)
{
    if (c->base.radix == 16 && letter < 6 && !calc_entry_is_object(c)) {
        calc_entry_digit(c, (uint8_t)(10 + letter));
        return;
    }
    if (letter >= 26)
        return;
    char ch = (char)('A' + letter);
    if (!c->editing)
        begin_entry(c);
    if (entry_has(c, '{'))
        return;

    bool all_letters = true;
    for (int i = 0; i < c->entry_len; i++)
        if (c->entry[i] < 'A' || c->entry[i] > 'Z')
            all_letters = false;
    char last = c->entry_len ? c->entry[c->entry_len - 1] : '\0';
    bool unit = (ch == 'D' || ch == 'H' || ch == 'M' || ch == 'S');

    if (all_letters && c->entry_len < 2)
        calc_entry_append(c, ch);                         /* a tag: D, DT, TZ, ... */
    else if (unit && last >= '0' && last <= '9')
        calc_entry_append(c, ch);                         /* 1D2H, 90M, ... */
}

