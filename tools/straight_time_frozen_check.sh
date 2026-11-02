#!/bin/sh
set -eu
mkdir -p /tmp/straight-frozen /tmp/straight-compiler
python3 -c "import zipfile;zipfile.ZipFile('/inputs/source.zip').extractall('/tmp/straight-frozen')"
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/straight-compiler
cd /tmp/straight-frozen
export JIELI_TOOLCHAIN=/tmp/straight-compiler/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk
export JIELI_DOCKER=0
export X0X_TIMING_RATE_HZ=44100
python3 tools/build.py --release 0.1.32-straightbeta --sdk /sdk >/work/build/straight-time/frozen-rebuild.log 2>&1
cmp build/x0x-0.1.32-straightbeta.fwsc /inputs/expected.fwsc
cmp build/x0x.bin /work/build/straight-time/target/x0x.bin
printf 'STRAIGHT_FROZEN_SOURCE_BYTE_IDENTICAL_PASS\n'
