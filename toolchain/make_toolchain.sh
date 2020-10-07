#!/bin/sh

set -e
set -x

SYSROOT="$(realpath ../root)"

sed "s|@@sysroot@@|$SYSROOT|g" < cross_file.txt.in > cross_file.txt

PREFIX="$(realpath ../cross-root)"
TARGET=i686-echidnaos
BINUTILSVERSION=2.35
GCCVERSION=10.2.0

if [ -z "$MAKEFLAGS" ]; then
	MAKEFLAGS="$1"
fi
export MAKEFLAGS

export PATH="$PREFIX/bin:$PATH"

if [ ! -f binutils-$BINUTILSVERSION.tar.gz ]; then
    wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILSVERSION.tar.gz
fi
if [ ! -f gcc-$GCCVERSION.tar.gz ]; then
    wget https://ftp.gnu.org/gnu/gcc/gcc-$GCCVERSION/gcc-$GCCVERSION.tar.gz
fi
if [ ! -f automake-1.15.1.tar.gz ]; then
    wget https://ftp.gnu.org/gnu/automake/automake-1.15.1.tar.gz
fi

rm -rf build
mkdir build
cd build

tar -xf ../binutils-$BINUTILSVERSION.tar.gz
git clone https://github.com/managarm/mlibc.git
tar -xf ../gcc-$GCCVERSION.tar.gz
tar -xf ../automake-1.15.1.tar.gz

mkdir build-automake
cd build-automake
../automake-1.15.1/configure --prefix="$PREFIX"
make
make install
cd ..

cd binutils-$BINUTILSVERSION
patch -p1 < ../../binutils.patch
cd ld
automake
cd ../..
mkdir build-binutils
cd build-binutils
../binutils-$BINUTILSVERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-nls --disable-werror
make
make install
cd ..

cd mlibc
git checkout 25eeedab0c0622ac7223b3139c796c955b42e5a6
cd ..
cp -rv mlibc mlibc-orig
cd mlibc
patch -p2 < ../../mlibc.patch
cd ..
mkdir build-mlibc
cd build-mlibc
meson --cross-file ../../cross_file.txt --prefix=/usr -Dheaders_only=true ../mlibc
ninja
DESTDIR="$SYSROOT" ninja install
cd ..

cd gcc-$GCCVERSION
patch -p1 < ../../gcc.patch
contrib/download_prerequisites
cd libstdc++-v3
autoconf
cd ../..
mkdir build-gcc
cd build-gcc
mkdir -p "$SYSROOT/usr/include"
../gcc-$GCCVERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot="$SYSROOT" --disable-nls --enable-languages=c,c++ --disable-shared --disable-gcov --disable-multilib --enable-initfini-array
make all-gcc
make install-gcc
cd ..

rm -rf build-mlibc
mkdir build-mlibc
cd build-mlibc
meson --cross-file ../../cross_file.txt --prefix=/usr --libdir=lib --buildtype=debugoptimized -Dstatic=true -Dmlibc_no_headers=true ../mlibc
ninja
DESTDIR="$SYSROOT" ninja install
cd ..

cd build-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-target-libgcc
make install-target-libstdc++-v3
cd ..
