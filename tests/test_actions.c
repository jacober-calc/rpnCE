/* Preserve every menu -> command/argument mapping, including 2ND behavior.
 * Baseline actions.c was recompiled against the reordered menu IDs to
 * confirm only ordering changed when TVM and HELP swapped places. */
#include <inttypes.h>
#include <stdio.h>
#include "actions.h"
#include "res.h"

uint8_t mock_res[65536];
size_t mock_res_size;

int main(void)
{
    FILE *f = fopen("data/RPNCEres.bin", "rb");
    if (!f) return 1;
    mock_res_size = fread(mock_res, 1, sizeof mock_res, f);
    fclose(f);
    if (!res_open()) return 1;

    uint64_t fingerprint = UINT64_C(14695981039346656037);
    for (unsigned id = 0; id < MID_COUNT; id++) {
        for (int shifted = 0; shifted < 2; shifted++) {
            uint8_t arg = 255;
            calc_cmd_t cmd = action_for((menu_id_t)id, shifted != 0, &arg);
            fingerprint ^= (unsigned)cmd;
            fingerprint *= UINT64_C(1099511628211);
            fingerprint ^= arg;
            fingerprint *= UINT64_C(1099511628211);
        }
    }
    int fails = fingerprint != UINT64_C(0x7552b9b30528ed15);
    printf("%s %u menu IDs, plain and shifted: %016" PRIx64 "\n",
           fails ? "FAIL" : "ok  ", (unsigned)MID_COUNT, fingerprint);
    printf("%d failures\n", fails);
    return fails != 0;
}
