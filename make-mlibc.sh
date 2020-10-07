#!/bin/bash

set -e
set -x

export PATH="$(realpath cross-root/bin):$PATH"

SYSROOT="$(realpath root)"

cd toolchain/build
rm -rf mlibc/cxxshim mlibc/frigg

git diff --no-index mlibc-orig mlibc > ../mlibc.patch || true

rm -rf build-mlibc
mkdir build-mlibc
cd build-mlibc

meson --cross-file ../../cross_file.txt --prefix=/usr --libdir=lib --buildtype=debugoptimized -Dstatic=true -Dmlibc_no_headers=true ../mlibc
ninja
DESTDIR="$SYSROOT" ninja install
