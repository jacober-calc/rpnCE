#!/bin/sh
# Builds a release: release/RPNCE-<version>.zip with the program, its
# RPNCEres appvar, and install notes. Run from the project root:
#   sh tools/release.sh
# Needs the CE toolchain (make) and zip.
set -e
VERSION=$(sed -n 's/#define RPNCE_VERSION "\(.*\)"/\1/p' src/core/version.h)
[ -n "$VERSION" ] || { echo "can't read the version from src/core/version.h"; exit 1; }

make
DIR=release/RPNCE-$VERSION
rm -rf "$DIR" "$DIR.zip"
mkdir -p "$DIR"
cp bin/RPNCE.8xp bin/RPNCEres.8xv "$DIR/"
sed "s/@VERSION@/$VERSION/" docs/INSTALL.txt > "$DIR/INSTALL.txt"
(cd release && zip -q -r "RPNCE-$VERSION.zip" "RPNCE-$VERSION")

# keep prod/ (the current build in the repo) up to date too
mkdir -p prod
cp "$DIR/RPNCE.8xp" "$DIR/RPNCEres.8xv" "$DIR/INSTALL.txt" prod/
echo "release/RPNCE-$VERSION.zip:"
ls -l "$DIR"
