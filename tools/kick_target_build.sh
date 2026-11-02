# Historical0.1.37 helper; currentreviewbuild: tools/build_release_060.py.
#!/bin/sh
# Run only inside the pinned, offline builder with /inputs/compiler.tar.xz and /sdk.
set -eu
mkdir -p /tmp/phase-toolchain /tmp/kickrecall-target
tar --no-same-owner -xf /inputs/compiler.tar.xz -C /tmp/phase-toolchain
cp -R /src/. /tmp/kickrecall-target/
cd /tmp/kickrecall-target
export JIELI_TOOLCHAIN=/tmp/phase-toolchain/jieli-linux-toolchains-20260730.1
export AC79_SDK=/sdk JIELI_DOCKER=0
if python3 tools/build.py --release 0.1.37-kickrecall --sdk /sdk >/tmp/kick-build.log 2>&1; then
 mkdir -p /src/build/kick-target
 cp build/x0x.bin build/x0x.elf build/x0x.dis build/x0x-0.1.37-kickrecall.fwsc /src/build/kick-target/
 cp /tmp/kick-build.log /src/build/kick-target-build.log
 tail -18 /tmp/kick-build.log
else
 cp /tmp/kick-build.log /src/build/kick-target-build.log
 tail -35 /tmp/kick-build.log
 exit 1
fi
