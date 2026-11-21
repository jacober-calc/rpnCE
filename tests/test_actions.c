/* Menu command/argument mappings, including 2ND behavior. */
#include <stdio.h>
#include "actions.h"
#include "res.h"
#include "action_contract.h"

uint8_t mock_res[65536];
size_t mock_res_size;

int main(void)
{
    FILE *f = fopen("data/rpnCEres.bin", "rb");
    if (!f) return 1;
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (!res_open()) return 1;

    int fails = action_contract() != ACTION_CONTRACT_EXPECTED;
    printf("%s %u menu IDs, plain and shifted\n",
           fails ? "FAIL" : "ok  ", (unsigned)MID_COUNT);
    for (int comma = 0; comma <= 1; comma++) {
        calc_t c = {0};
        c.decimal_comma = comma != 0;
        for (int shifted = 0; shifted <= 1; shifted++) {
            uint8_t arg;
            if (action_for(MID_DECIMAL_POINT, shifted, &arg) != CMD_DECIMAL_POINT ||
                action_for(MID_DECIMAL_COMMA, shifted, &arg) != CMD_DECIMAL_COMMA ||
                action_is_active(&c, MID_DECIMAL_POINT) != !comma ||
                action_is_active(&c, MID_DECIMAL_COMMA) != !!comma)
                fails++;
        }
    }
    static const struct { menu_id_t id; calc_cmd_t cmd; } complex_actions[] = {
        {MID_COMPLEX_REAL,CMD_COMPLEX_REAL}, {MID_COMPLEX_IMAG,CMD_COMPLEX_IMAG},
        {MID_COMPLEX_CONJ,CMD_COMPLEX_CONJ}, {MID_COMPLEX_ABS,CMD_COMPLEX_ABS},
        {MID_COMPLEX_ANGLE,CMD_COMPLEX_ANGLE}, {MID_NUM_RESULT_MODE_REAL,CMD_RESULT_REAL},
        {MID_NUM_RESULT_MODE_COMPLEX,CMD_RESULT_COMPLEX}, {MID_COMPLEX_MODE_RECT,CMD_COMPLEX_RECT},
        {MID_COMPLEX_MODE_RAD,CMD_COMPLEX_RAD}, {MID_COMPLEX_MODE_DEG,CMD_COMPLEX_DEG},
    };
    for (unsigned i = 0; i < sizeof complex_actions / sizeof *complex_actions; i++)
        for (int shifted = 0; shifted <= 1; shifted++) {
            uint8_t arg = 255;
            if (action_for(complex_actions[i].id, shifted, &arg) != complex_actions[i].cmd || arg) fails++;
        }
    for (int mode = COMPLEX_RECT; mode <= COMPLEX_DEG; mode++) {
        calc_t c = {0}; c.complex_mode = mode;
        if (action_is_active(&c,MID_COMPLEX_MODE_RECT) != (mode == COMPLEX_RECT) ||
            action_is_active(&c,MID_COMPLEX_MODE_RAD) != (mode == COMPLEX_RAD) ||
            action_is_active(&c,MID_COMPLEX_MODE_DEG) != (mode == COMPLEX_DEG)) fails++;
    }
    /* Bounds are checked before looking up any generated entry. */
    const menu_id_t invalid[] = { (menu_id_t)-1, MID_COUNT, (menu_id_t)UINT16_MAX };
    calc_t c = {0};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        uint8_t arg = 255;
        if (action_for(invalid[i], true, &arg) != CMD_NONE || arg ||
            action_is_active(&c, invalid[i])) fails++;
    }
    printf("%d failures\n", fails);
    return fails != 0;
}
