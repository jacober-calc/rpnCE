#include "keys.h"

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

/* Key layout follows RPN83P where it has a meaning on the CE keypad. */
calc_cmd_t keys_translate(keys_t *k, sk_key_t key, uint8_t *arg)
{
    *arg = 0;

    if (key == sk_2nd) {
        k->second = !k->second;
        return CMD_NONE;
    }

    bool second = k->second;
    k->second = false;

    if (second) {
        switch (key) {
            case sk_Mode:   k->quit = true; return CMD_NONE;  /* QUIT */
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
            case sk_Enter:  return CMD_SHOW;                  /* ENTRY */
            default:        return CMD_NONE;
        }
    }

    int d = digit_of(key);
    if (d >= 0) {
        *arg = (uint8_t)d;
        return CMD_DIGIT;
    }

    switch (key) {
        case sk_DecPnt: return CMD_DOT;
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
