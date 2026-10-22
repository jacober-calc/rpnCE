#!/bin/sh
# Check and package a candidate. Add --acceptance <completed JSON> for
# a final package and prod/ update. See docs/RELEASING.md.
set -eu
cd "$(dirname "$0")/.."
PYTHON=${PYTHON:-python3}
"$PYTHON" tools/release_check.py
"$PYTHON" tools/package_release.py "$@"
