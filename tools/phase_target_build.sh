#!/bin/sh
# Run only inside the pinned, offline builder with /inputs/compiler.tar.xz and /sdk.
set -eu
mkdir -p /tmp/phase-toolchain /tmp/phase-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/phase-toolchain
cp -R /src/. /tmp/phase-target/
cd /tmp/phase-target
export JIELI_TOOLCHAIN=/tmp/phase-toolchain/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk JIELI_DOCKER=0
if python3 tools/build.py --release 0.1.33-phasebeta --sdk /sdk >/tmp/phase-build.log 2>&1; then
 mkdir -p /src/build/phase-target
 cp build/x0x.bin build/x0x.elf build/x0x.dis build/x0x-0.1.33-phasebeta.fwsc /src/build/phase-target/
 cp /tmp/phase-build.log /src/build/phase-target-build.log
 tail -18 /tmp/phase-build.log
else
 cp /tmp/phase-build.log /src/build/phase-target-build.log
 tail -35 /tmp/phase-build.log
 exit 1
fi
