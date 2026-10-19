#!/bin/sh
# Compare both cores in one process. Requires a Git checkout containing the
# baseline commit; the regular tests/run_tests.sh also works from a ZIP.
set -eu
CC=${CC:-cc}
BASELINE=${1:-08913263794c9ad04d0db0983a2473e9dd6b2d92}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

git show "$BASELINE:src/core/calc.c" > "$OUT/original_calc.c"
# EXTRA_CFLAGS can enable e.g. -fsanitize=address,undefined for both cores.
$CC ${EXTRA_CFLAGS:-} -DRPNCE_HOST_TEST -Itests/mock -Isrc/core \
    -Dcalc_init=original_calc_init -Dcalc_execute=original_calc_execute \
    -Dcalc_register=original_calc_register -Dcalc_finish_entry=original_calc_finish_entry \
    -Dcalc_seed_random=original_calc_seed_random \
    -c "$OUT/original_calc.c" -o "$OUT/original_calc.o"
$CC ${EXTRA_CFLAGS:-} -DRPNCE_HOST_TEST -Itests/mock -Isrc/core \
    tests/test_core_equivalence.c tests/mock_calc_runtime.c "$OUT/original_calc.o" \
    src/core/calc.c src/core/calc_entry.c src/core/calc_date.c src/core/calc_quantity.c \
    src/core/mathx.c src/core/stats.c src/core/tvm.c \
    src/core/units.c tests/units_data.c src/core/base.c src/core/datetime.c \
    -lm -o "$OUT/equivalence"
"$OUT/equivalence"
