#!/bin/bash

set -ex

[ -d mlibc-workdir ] || ( \
    mkdir mlibc-workdir && \
    tar -xf mlibc.tar.gz -C mlibc-workdir --strip-components=1 && \
    cd mlibc-workdir && \
    patch -p2 < ../patches/mlibc/mlibc.patch )

[ -d mlibc-orig ] || ( \
    mkdir mlibc-orig && \
    tar -xf mlibc.tar.gz -C mlibc-orig --strip-components=1 )

git diff --no-index mlibc-orig mlibc-workdir > patches/mlibc/mlibc.patch || true

rm -rf mlibc
cd build
xbstrap install -u mlibc
