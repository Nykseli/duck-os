#!/bin/bash

# Building cross compliation based on https://wiki.osdev.org/GCC_Cross-Compiler

set -e

cd $(dirname ${BASH_SOURCE[0]})


mkdir -p cross-compile
cd cross-compile
CROSS_ROOT=$PWD

export PREFIX="$PWD/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

BIN_UTIL_NAME=binutils-2.44
GCC_NAME=gcc-15.1.0

if [[ ! -f "$BIN_UTIL_NAME.tar.gz" ]]; then
    wget "https://ftpmirror.gnu.org/gnu/binutils/$BIN_UTIL_NAME.tar.gz"
else
    echo "$BIN_UTIL_NAME.tar.gz exists. remove it to redownload it"
fi

if [[ ! -d "$BIN_UTIL_NAME" ]]; then
    tar -xf "$BIN_UTIL_NAME.tar.gz"
else
    echo "$BIN_UTIL_NAME exists. remove it to redownload it"
fi

if [[ ! -d build-binutils ]]; then
    mkdir build-binutils
    cd build-binutils
    ../$BIN_UTIL_NAME/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
    make -j$(nproc)
    make -j$(nproc) install
else
    echo binutils is built. remove build-binutils to rebuild
fi

cd $CROSS_ROOT

which -- $TARGET-as || echo $TARGET-as is not in the PATH

if [[ ! -f "$GCC_NAME.tar.xz" ]]; then
    wget "https://ftpmirror.gnu.org/gnu/gcc/$GCC_NAME/$GCC_NAME.tar.xz"
else
    echo "$GCC_NAME.tar.xz exists. remove it to redownload it"
fi

if [[ ! -d "$GCC_NAME" ]]; then
    tar -xf "$GCC_NAME.tar.xz"
else
    echo "$GCC_NAME exists. remove it to redownload it"
fi

if [[ ! -d build-gcc ]]; then
    mkdir build-gcc
    cd build-gcc
    ../$GCC_NAME/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
    make -j$(nproc) all-gcc
    make -j$(nproc) all-target-libgcc
    make -j$(nproc) all-target-libstdc++-v3
    make -j$(nproc) install-gcc
    make -j$(nproc) install-target-libgcc
    make -j$(nproc) install-target-libstdc++-v3
else
    echo gcc is built. remove build-gcc to rebuild
fi

# TODO: GDB
