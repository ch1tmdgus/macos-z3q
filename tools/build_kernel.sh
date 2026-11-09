#!/bin/bash
set -euo pipefail
if [[ $# != 2 ]]; then echo "Usage: $0 PREPARED_XNU_TREE PREPARED_XNU_DEPS" >&2; exit 2; fi
export XNU_SRC=$(cd "$1" && pwd)
export XNUDEPS=$(cd "$2" && pwd)
export PATH="$XNUDEPS/host/bin:$PATH"
export XNU_SDK_PATH="$XNUDEPS/sdk/iPhoneOS16.5.xnu.sdk"
export XNU_SDK_VERSION=16.5
export XNU_PLATFORM_PATH="$XNUDEPS/sdk/Platforms/iPhoneOS.platform"
export RC_DARWIN_KERNEL_VERSION=8796.121.2
MAC_SDK=$(/usr/bin/xcrun --sdk macosx --show-sdk-path)
cd "$XNU_SRC"
exec make MEMORY_SIZE=19327352832 SYSCTL_HW_PHYSICALCPU=8 KERNEL_BUILDS_IN_PARALLEL=1 SDKROOT=iphoneos ARCH_CONFIGS=ARM64 MACHINE_CONFIGS=BCM2837 KERNEL_CONFIGS=DEVELOPMENT ARCH_STRING_FOR_CURRENT_MACHINE_CONFIG=arm64 BUILD_WERROR=0 BUILD_LTO=0 DO_CTFMERGE=0 BUILD_DSYM=0 HOST_SDKROOT_RESOLVED="$MAC_SDK" HOST_CC="/usr/bin/clang -isysroot $MAC_SDK -Wno-unused-command-line-argument" HOST_FLEX=/usr/bin/flex HOST_BISON=/usr/bin/bison HOST_GM4=/usr/bin/m4 CC=xnu-clang CXX=xnu-clang++ LOGCOLORS=n
