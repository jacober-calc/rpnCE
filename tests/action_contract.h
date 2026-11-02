#ifndef RPNCE_TEST_ACTION_CONTRACT_H
#define RPNCE_TEST_ACTION_CONTRACT_H

#include "actions.h"

#define ACTION_CONTRACT_EXPECTED UINT32_C(0x70D73887)

/* Complete command/argument mapping, including shifted TVM recall. Integer
 * widths and any representation chosen by the generator do not affect it. */
static uint32_t action_contract(void)
{
    uint32_t hash = UINT32_C(2166136261);
    for (uint16_t id = 0; id < MID_COUNT; id++) {
        for (uint8_t shifted = 0; shifted < 2; shifted++) {
            uint8_t arg;
            uint16_t cmd = (uint16_t)action_for((menu_id_t)id, shifted != 0, &arg);
            hash = (hash ^ (uint8_t)cmd) * UINT32_C(16777619);
            hash = (hash ^ (uint8_t)(cmd >> 8)) * UINT32_C(16777619);
            hash = (hash ^ arg) * UINT32_C(16777619);
        }
    }
    return hash;
}

#endif
