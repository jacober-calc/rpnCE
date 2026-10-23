/* Entry, stack and record contracts. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calc.h"
#include "dateobj.h"
#include "units.h"

static unsigned steps;
static int fails;

static void check(const char *name, bool pass)
{
    if (!pass) {
        fprintf(stderr, "FAIL %s at step %u\n", name, steps);
        fails++;
    }
}

static void invariant(const calc_t *c)
{
    check("stack depth", c->depth >= STACK_MIN && c->depth <= STACK_MAX);
    check("register count", c->extra_count <= REG_EXTRA_MAX);
    check("entry bounds", c->entry_len <= ENTRY_MAX &&
          c->entry[c->entry_len] == '\0' && strlen(c->entry) == c->entry_len);
    check("BASE settings", (c->base.radix == 2 || c->base.radix == 8 ||
          c->base.radix == 10 || c->base.radix == 16) &&
          c->base.bits >= 1 && c->base.bits <= BASE_MAX_BITS);
    for (int i = 0; i < STACK_MAX; i++) {
        check("stack unit tag", c->unit[i] <= UNIT_COUNT);
        if (obj_is(&c->stack[i])) {
            dt_obj_t object = obj_unpack(&c->stack[i]);
            check("DATE object", !c->unit[i] && dt_validate(&object) == DT_OK);
        }
    }
}

static void step(calc_t *c, calc_cmd_t cmd, uint8_t arg)
{
    calc_execute(c, cmd, arg);
    steps++;
    invariant(c);
}

static void trace(calc_t *c, const char *text)
{
    for (; *text; text++) {
        calc_cmd_t cmd;
        uint8_t arg = 0;
        switch (*text) {
            case '=': cmd = CMD_ENTER; break;
            case '~': cmd = CMD_BACKSPACE; break;
            case '!': cmd = CMD_CLEAR; break;
            case '-': cmd = CMD_CHS; break;
            case '.': cmd = CMD_DOT; break;
            case ',': case 'E': cmd = CMD_EE; break;
            case '{': cmd = CMD_RECORD_OPEN; break;
            case '}': cmd = CMD_RECORD_CLOSE; break;
            case '+': cmd = CMD_ADD; break;
            case '*': cmd = CMD_MUL; break;
            default:
                if (*text >= '0' && *text <= '9') {
                    cmd = CMD_DIGIT; arg = (uint8_t)(*text - '0');
                } else {
                    cmd = CMD_ALPHA_LETTER; arg = (uint8_t)(*text - 'A');
                }
                break;
        }
        step(c, cmd, arg);
    }
}

static void examples(uint8_t depth)
{
    calc_t c;
    calc_init(&c); c.depth = depth;
    trace(&c, "2=3+4*");
    check("chained arithmetic", !c.message && c.stack[0].v == 20 &&
          c.lastx.v == 4 && c.lift && !c.editing);
    calc_init(&c); c.depth = depth;
    trace(&c, "2===***");
    check("repeating top register", !c.message && c.stack[0].v == 16);
    calc_init(&c); c.depth = depth;
    trace(&c, "1234~~5=");
    check("backspace during entry", !c.message && c.stack[0].v == 125 &&
          c.stack[1].v == 125 && !c.lift);
    calc_init(&c); c.depth = depth;
    trace(&c, "{2024,2,30=");
    check("invalid record stays editable", c.editing && c.message &&
          !strcmp(c.entry, "{2024,2,30"));
    trace(&c, "~~29}=");
    dt_obj_t date = obj_unpack(&c.stack[0]);
    check("record repair", !c.message && !c.editing && obj_is(&c.stack[0]) &&
          date.type == OBJ_DATE && date.year == 2024 && date.month == 2 && date.day == 29);
    calc_init(&c); c.depth = depth;
    trace(&c, "12345678901234567890E-123=");
    check("entry digit limits", !strcmp(c.entry, "123456789012E-12") && !c.editing);
}

int main(void)
{
    static const char *const traces[] = {
        "12.34567890123456789E-123~~5=!!-0.25E-9=",
        "{2024,2,30=~~29}=!!DT{2024,3,14,15,36,1}=",
        "DZ{2024,3,14,15,36,1,-7,0}=}", "1D3H10M=!!-2D5M=",
        "D{2024,3,14}=10+!!T{23,59,59}=", "{2024,12,31}~=!{{!!123~~~-5===!!!",
    };
    static const calc_cmd_t commands[] = {
        CMD_DIGIT, CMD_DIGIT, CMD_DIGIT, CMD_DOT, CMD_EE, CMD_CHS,
        CMD_BACKSPACE, CMD_CLEAR, CMD_ENTER, CMD_RECORD_OPEN,
        CMD_RECORD_CLOSE, CMD_ALPHA_LETTER, CMD_RECORD_TAG, CMD_BASE_RADIX
    };
    for (uint8_t depth = STACK_MIN; depth <= STACK_MAX; depth += STACK_MAX - STACK_MIN) {
        examples(depth);
        for (unsigned t = 0; t < sizeof traces / sizeof *traces; t++) {
            calc_t c;
            calc_init(&c); c.depth = depth;
            trace(&c, traces[t]);
        }
        calc_t c;
        calc_init(&c); c.depth = depth;
        uint32_t state = UINT32_C(0x52504e43);
        for (unsigned i = 0; i < 4096; i++) {
            state ^= state << 13; state ^= state >> 17; state ^= state << 5;
            calc_cmd_t cmd = commands[state % (sizeof commands / sizeof *commands)];
            uint8_t arg = (uint8_t)((state >> 8) % 10);
            if (cmd == CMD_RECORD_TAG) arg = (uint8_t)(1 + (state >> 8) % 7);
            if (cmd == CMD_ALPHA_LETTER) arg = (uint8_t)((state >> 8) % 26);
            if (cmd == CMD_BASE_RADIX) {
                const uint8_t radices[] = {2, 8, 10, 16};
                arg = radices[(state >> 8) % 4];
            }
            step(&c, cmd, arg);
        }
    }
    printf("%u entry/stack steps checked\n%d failures\n", steps, fails);
    return fails != 0;
}
