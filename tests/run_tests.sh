#!/bin/sh
# Builds and runs every PC test suite. Run from the project root:
#   sh tests/run_tests.sh
# Needs a C compiler (gcc or clang) and the math library.
set -e
CC=${CC:-cc}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
M="-Itests/mock -Isrc/core"
fail=0

run() {
    name=$1; shift
    if ! $CC -w "$@" -o "$OUT/$name" -lm; then
        echo "$name: did not build"; fail=1; return
    fi
    if ! "$OUT/$name" > "$OUT/$name.log"; then
        echo "$name: failed"
        cat "$OUT/$name.log"; fail=1; return
    fi
    result=$(tail -1 "$OUT/$name.log")
    printf '%-10s %s\n' "$name" "$result"
    case "$result" in "0 failures") ;; *) fail=1 ;; esac
}

run format   tests/test_format.c src/core/format.c
run rounding -Itests/bcd -Isrc/core tests/test_rounding.c src/core/rounding.c
run mathx    $M tests/test_mathx.c src/core/mathx.c
run base     $M tests/test_base.c src/core/base.c
run stats    $M tests/test_stats.c src/core/stats.c src/core/mathx.c
run tvm      $M tests/test_tvm_units.c src/core/tvm.c src/core/mathx.c \
             src/core/units.c tests/units_data.c
run datetime -Isrc/core tests/test_datetime.c src/core/datetime.c
run res      -Itests/mock_fileioc -Itests/mock -Isrc/ui -Isrc/core tests/test_res.c \
             src/ui/res.c
run actions  -Itests/mock_fileioc $M -Isrc/ui tests/test_actions.c \
             src/ui/actions.c src/ui/res.c
run keys     -Itests/mock_fileioc $M -Isrc/ui tests/test_keys.c src/ui/keys.c \
             src/ui/menu.c src/ui/res.c
run dotfont  -Itests/mock_graphx -Isrc/ui tests/test_dotfont.c \
             src/ui/dotfont.c
run storage  -Itests/mock_storage $M tests/test_storage.c \
             src/sys/storage.c src/core/base.c src/core/tvm.c src/core/mathx.c \
             src/core/datetime.c
run core     -DRPNCE_HOST_TEST $M tests/test_calc.c tests/mock_calc_runtime.c src/core/calc.c \
             src/core/calc_entry.c src/core/calc_date.c src/core/calc_quantity.c \
             src/core/mathx.c src/core/stats.c src/core/tvm.c src/core/units.c \
             tests/units_data.c src/core/base.c src/core/datetime.c
run fixes    -DRPNCE_HOST_TEST $M -Isrc/ui tests/test_fixes.c tests/mock_calc_runtime.c \
             src/ui/catalog.c src/core/calc.c src/core/calc_entry.c src/core/calc_date.c \
             src/core/calc_quantity.c src/core/mathx.c src/core/stats.c src/core/tvm.c \
             src/core/units.c tests/units_data.c src/core/base.c src/core/datetime.c
run display  -DRPNCE_HOST_TEST -Itests/mock_graphx -Itests/mock_fileioc $M -Isrc/ui \
             tests/test_display.c tests/mock_calc_runtime.c \
             src/ui/display.c src/ui/dotfont.c src/ui/menu.c src/ui/catalog.c \
             src/ui/actions.c src/ui/res.c src/core/calc.c src/core/calc_entry.c \
             src/core/calc_date.c src/core/calc_quantity.c src/core/base.c src/core/datetime.c src/core/mathx.c \
             src/core/stats.c src/core/tvm.c src/core/units.c

[ $fail -eq 0 ] && echo "all tests passed" || { echo "some tests failed"; exit 1; }
