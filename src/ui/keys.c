#include <string.h>
#include "keys.h"
#include "menu_data.h"

/* The five keys under the menu row: Y= WINDOW ZOOM TRACE GRAPH. */
static int softkey_of(sk_key_t key)
{
    switch (key) {
        case sk_Yequ:   return 0;
        case sk_Window: return 1;
        case sk_Zoom:   return 2;
        case sk_Trace:  return 3;
        case sk_Graph:  return 4;
        default:        return -1;
    }
}

static int digit_of(sk_key_t key)
{
    switch (key) {
        case sk_0: return 0;  case sk_1: return 1;  case sk_2: return 2;
        case sk_3: return 3;  case sk_4: return 4;  case sk_5: return 5;
        case sk_6: return 6;  case sk_7: return 7;  case sk_8: return 8;
        case sk_9: return 9;
        default:   return -1;
    }
}

/* The green ALPHA letters printed above the CE keys. */
static int letter_index(sk_key_t key);

/* The register named by a key in a STO/RCL/x<> prompt: its green ALPHA
 * letter (A = 1 ... Z = 26), or -1. */
static int letter_of(sk_key_t key)
{
    int i = letter_index(key);
    return i < 0 ? -1 : i + 1;
}

static int letter_index(sk_key_t key)
{
    switch (key) {
        case sk_Math:   return 0;   /* A */
        case sk_Apps:   return 1;   /* B */
        case sk_Prgm:   return 2;   /* C */
        case sk_Recip:  return 3;   /* D */
        case sk_Sin:    return 4;   /* E */
        case sk_Cos:    return 5;   /* F */
        case sk_Tan:    return 6;   /* G */
        case sk_Power:  return 7;   /* H */
        case sk_Square: return 8;   /* I */
        case sk_Comma:  return 9;   /* J */
        case sk_LParen: return 10;  /* K */
        case sk_RParen: return 11;  /* L */
        case sk_Div:    return 12;  /* M */
        case sk_Log:    return 13;  /* N */
        case sk_7:      return 14;  /* O */
        case sk_8:      return 15;  /* P */
        case sk_9:      return 16;  /* Q */
        case sk_Mul:    return 17;  /* R */
        case sk_Ln:     return 18;  /* S */
        case sk_4:      return 19;  /* T */
        case sk_5:      return 20;  /* U */
        case sk_6:      return 21;  /* V */
        case sk_Sub:    return 22;  /* W */
        case sk_Store:  return 23;  /* X */
        case sk_1:      return 24;  /* Y */
        case sk_2:      return 25;  /* Z */
        default:        return -1;
    }
}

static uint8_t op_of(sk_key_t key)
{
    switch (key) {
        case sk_Add: return 1;
        case sk_Sub: return 2;
        case sk_Mul: return 3;
        case sk_Div: return 4;
        default:     return 0;
    }
}

static void update_prompt_text(keys_t *k)
{
    static const char ops[] = { 0, '+', '-', MSYM_TIMES, '/' };
    static const char names[][5] = { "", "STO", "RCL", "X<>", "FIX", "SCI", "ENG",
                                     "RNDN", "RNDG" };
    char *p = k->prompt_text;
    strcpy(p, names[k->prompt]);
    p += strlen(p);
    if (k->prompt_op)
        *p++ = ops[k->prompt_op];
    *p++ = ' ';
    if (k->prompt >= PROMPT_FIX && k->prompt_count)
        *p++ = '.';                         /* "FIX ._" after the . key */
    *p++ = '_';
    *p = '\0';
}

void keys_open_prompt(keys_t *k, prompt_kind_t kind)
{
    k->prompt = kind;
    k->prompt_op = 0;
    k->prompt_alpha = false;
    k->prompt_second = false;
    k->prompt_count = 0;
    update_prompt_text(k);
}

bool keys_cancel_prompt(keys_t *k)
{
    bool was_open = k->prompt != PROMPT_NONE;
    k->prompt = PROMPT_NONE;
    return was_open;
}

const char *keys_prompt_text(const keys_t *k)
{
    return k->prompt == PROMPT_NONE ? NULL : k->prompt_text;
}

const char *keys_take_flash(keys_t *k, bool *lifts)
{
    if (!k->flash)
        return NULL;
    k->flash = false;
    *lifts = k->flash_lifts;
    return k->flash_text;
}

/* Records the finished prompt: the prompt text with its "_" replaced. */
static void set_flash(keys_t *k, const char *answer)
{
    k->flash_lifts = keys_prompt_lifts(k);
    strcpy(k->flash_text, k->prompt_text);
    char *u = strchr(k->flash_text, '_');
    if (u)
        *u = '\0';
    strcat(k->flash_text, answer);
    k->flash = true;
}

bool keys_prompt_lifts(const keys_t *k)
{
    return k->prompt == PROMPT_RCL && k->prompt_op == 0;
}

/* While a STO/RCL prompt is open: + - x / pick the arithmetic (first key
 * only), letter keys pick the variable, CLEAR/DEL cancel. Once an operation
 * is chosen, or after ALPHA, the + - x / keys are their letters (W, R, M). */
static event_t prompt_key(keys_t *k, sk_key_t key)
{
    event_t ev = { EV_REDRAW, CMD_NONE, 0, false };

    if (key == sk_Clear || key == sk_Del) {
        k->prompt = PROMPT_NONE;
        return ev;
    }
    if (key == sk_2nd) {
        k->prompt_second = !k->prompt_second;
        return ev;
    }
    /* FIX / SCI / ENG / RNDN / RNDG, HP-32SII style: a digit 0-9 completes
     * the prompt; . then a digit means 10-19 (FIX . 1 is FIX 11). */
    if (k->prompt >= PROMPT_FIX) {
        int d = digit_of(key);
        if (key == sk_DecPnt && k->prompt_count == 0) {
            k->prompt_count = 1;                /* "FIX ._" */
            update_prompt_text(k);
            return ev;
        }
        if (d < 0) {
            ev.kind = EV_NONE;
            return ev;
        }
        uint8_t value = (uint8_t)(k->prompt_count ? 10 + d : d);
        char ans[3];
        if (value >= 10) {
            ans[0] = '1';
            ans[1] = (char)('0' + value - 10);
            ans[2] = '\0';
        } else {
            ans[0] = (char)('0' + value);
            ans[1] = '\0';
        }
        k->prompt_count = 0;                    /* flash shows the number: */
        k->prompt_second = false;
        update_prompt_text(k);                  /* "FIX 3", "FIX 11" */
        set_flash(k, ans);

        ev.kind = EV_CMD;
        static const calc_cmd_t cmds[] = { CMD_DISP_FIX, CMD_DISP_SCI, CMD_DISP_ENG,
                                           CMD_ROUND_N, CMD_ROUND_G };
        ev.cmd = cmds[k->prompt - PROMPT_FIX];
        ev.arg = value;
        k->prompt = PROMPT_NONE;
        return ev;
    }

    if (key == sk_Alpha) {
        k->prompt_alpha = true;
        return ev;
    }

    /* + - x / pick register arithmetic (STO and RCL only, first key only) */
    uint8_t op = op_of(key);
    if (op && !k->prompt_op && !k->prompt_alpha && k->prompt != PROMPT_XCH) {
        k->prompt_op = op;
        update_prompt_text(k);
        return ev;
    }

    /* the register: a letter A-Z, . for i, or 2ND . for (i) */
    int v;
    if (key == sk_DecPnt)
        v = k->prompt_second ? REG_INDIRECT : REG_I;
    else
        v = letter_of(key);
    k->prompt_second = false;
    if (v < 0) {
        ev.kind = EV_NONE;
        return ev;
    }

    /* show the register briefly: "STO A", "RCL+ i", "X<> (i)" */
    char ans[4];
    if (v == REG_INDIRECT)
        strcpy(ans, "(i)");
    else if (v == REG_I)
        strcpy(ans, "i");
    else {
        ans[0] = (char)('A' + v - 1);
        ans[1] = '\0';
    }
    set_flash(k, ans);

    ev.kind = EV_CMD;
    if (k->prompt == PROMPT_XCH) {
        ev.cmd = CMD_XCH;
    } else {
        calc_cmd_t base = (k->prompt == PROMPT_STO) ? CMD_STO : CMD_RCL;
        ev.cmd = (calc_cmd_t)(base + k->prompt_op);
    }
    ev.arg = (uint8_t)v;
    k->prompt = PROMPT_NONE;
    return ev;
}

static calc_cmd_t second_cmd(sk_key_t key)
{
    switch (key) {
        case sk_Square: return CMD_SQRT;
        case sk_Log:    return CMD_ALOG;                  /* 10^x */
        case sk_Ln:     return CMD_EXP;                   /* e^x */
        case sk_Sin:    return CMD_ASIN;
        case sk_Cos:    return CMD_ACOS;
        case sk_Tan:    return CMD_ATAN;
        case sk_Power:  return CMD_PI;
        case sk_Div:    return CMD_E;
        case sk_Comma:  return CMD_EE;
        case sk_Chs:    return CMD_LASTX;                 /* ANS */
        case sk_LParen: return CMD_RECORD_OPEN;           /* {: starts a DATE record */
        case sk_RParen: return CMD_RECORD_CLOSE;          /* }: closes one, else DROP */
        case sk_Del:    return CMD_ENTER;                 /* INS: DUP */
        case sk_Enter:  return CMD_SHOW;                  /* ENTRY */
        default:        return CMD_NONE;
    }
}

static calc_cmd_t plain_cmd(sk_key_t key)
{
    switch (key) {
        case sk_DecPnt: return CMD_DOT;
        case sk_Comma:  return CMD_EE;
        case sk_Chs:    return CMD_CHS;
        case sk_Del:    return CMD_BACKSPACE;
        case sk_Clear:  return CMD_CLEAR;
        case sk_Enter:  return CMD_ENTER;
        case sk_Add:    return CMD_ADD;
        case sk_Sub:    return CMD_SUB;
        case sk_Mul:    return CMD_MUL;
        case sk_Div:    return CMD_DIV;
        case sk_Power:  return CMD_POW;
        case sk_Square: return CMD_SQUARE;
        case sk_Recip:  return CMD_RECIP;
        case sk_Log:    return CMD_LOG;
        case sk_Ln:     return CMD_LN;
        case sk_Sin:    return CMD_SIN;
        case sk_Cos:    return CMD_COS;
        case sk_Tan:    return CMD_TAN;
        case sk_LParen: return CMD_ROLL_DOWN;
        case sk_RParen: return CMD_SWAP;
        default:        return CMD_NONE;
    }
}

bool keys_brightness_key(keys_t *k, sk_key_t key, bool *brighter)
{
    if ((!k->second && !(k->prompt != PROMPT_NONE && k->prompt_second)) ||
        (key != sk_Up && key != sk_Down))
        return false;
    *brighter = key == sk_Up;
    k->second = k->prompt_second = false;
    return true;
}

event_t keys_translate(keys_t *k, sk_key_t key)
{
    event_t ev = { EV_NONE, CMD_NONE, 0, false };

    if (k->prompt != PROMPT_NONE)
        return prompt_key(k, key);

    if (key == sk_Alpha) {
        k->alpha = !k->alpha;
        k->second = false;
        ev.kind = EV_REDRAW;
        return ev;
    }
    if (k->alpha) {
        /* ALPHA + a key: its green letter. The core uses it as a HEX digit
         * (A-F in HEX mode), a DATE type tag, or a duration unit. Any key
         * without a letter just cancels ALPHA. */
        k->alpha = false;
        int idx = letter_index(key);
        if (idx >= 0) {
            ev.kind = EV_CMD;
            ev.cmd = CMD_ALPHA_LETTER;
            ev.arg = (uint8_t)idx;
            return ev;
        }
    }

    if (key == sk_2nd) {
        k->second = !k->second;
        ev.kind = EV_REDRAW;
        return ev;
    }

    bool second = k->second;
    k->second = false;

    if (key == sk_Store) {
        keys_open_prompt(k, second ? PROMPT_RCL : PROMPT_STO);
        ev.kind = EV_REDRAW;
        return ev;
    }

    int slot = softkey_of(key);
    if (slot >= 0) {
        ev.kind = EV_SOFTKEY;
        ev.arg = (uint8_t)slot;
        ev.shifted = second;            /* 2ND + menu key: recall (TVM) */
        return ev;
    }

    if (second) {
        if (key == sk_Mode) {
            ev.kind = EV_QUIT;
        } else if (key == sk_0) {               /* CATALOG */
            ev.kind = EV_HELP;
        } else if (key == sk_Add || key == sk_Stat) {   /* MEM, LIST */
            ev.kind = EV_MENU;
            ev.arg = key == sk_Add ? MENU_GROUP_CLEAR : MENU_GROUP_STACK;
        } else {
            /* 2ND LINK (on X,T,theta,n) and 2ND ANGLE (on APPS) are left
             * free on purpose: complex-number entry will need them. */
            ev.cmd = second_cmd(key);
            if (ev.cmd != CMD_NONE)
                ev.kind = EV_CMD;
        }
        return ev;
    }

    switch (key) {
        case sk_Up:     ev.kind = EV_ROW_UP; return ev;
        case sk_Left:   ev.kind = EV_SCROLL_LEFT; return ev;
        case sk_Right:  ev.kind = EV_SCROLL_RIGHT; return ev;
        case sk_Vars:   ev.kind = EV_VARS; return ev;
        case sk_Down:   ev.kind = EV_ROW_DOWN; return ev;
        case sk_Math:   ev.kind = EV_MENU; ev.arg = MENU_GROUP_HOME; return ev;
        case sk_Mode:   ev.kind = EV_MENU; ev.arg = MENU_GROUP_MODE; return ev;
        case sk_Stat:   ev.kind = EV_MENU; ev.arg = MENU_GROUP_STAT; return ev;
        case sk_GraphVar: ev.kind = EV_MENU; ev.arg = MENU_GROUP_UNIT; return ev;
        case sk_Apps:   ev.kind = EV_MENU; ev.arg = MENU_GROUP_BASE; return ev;
        case sk_Prgm:   ev.kind = EV_MENU; ev.arg = MENU_GROUP_HYPERBOLIC; return ev;
        default:        break;
    }

    int d = digit_of(key);
    if (d >= 0) {
        ev.kind = EV_CMD;
        ev.cmd = CMD_DIGIT;
        ev.arg = (uint8_t)d;
        return ev;
    }

    ev.cmd = plain_cmd(key);
    if (ev.cmd != CMD_NONE)
        ev.kind = EV_CMD;
    return ev;
}
