# Source and build notes

## Kernel source

Base: Apple XNU 8796.121.2, commit `c54f35ca767986246321eb901baf8f5ff7923f6a`. The patch and overlay together contain the edited kernel source. Pre-existing platform work came from the pinned Samsung/HackiOS base documented in THIRD_PARTY.md.

The generated `redwood_focaltech_fw.h` embeds firmware for an inherited, non-S20 touch driver. Its contents are deliberately omitted. Supply a matching local header from your existing inputs; the required digest is recorded in `manifests/external-inputs.json`. This build dependency does not mean the S20 uses that controller.

```sh
python3 tools/prepare_kernel.py --tree build/xnu --focaltech-header /path/to/redwood_focaltech_fw.h
```

This verifies the external header and clean Apple source hashes, applies the patch, copies the overlay, and verifies the resulting source hashes. `--existing` accepts an already prepared clean Apple source directory instead of cloning it. It never writes a phone.

## Native kernel build

A macOS/Xcode host and an already prepared HackiOS XNU dependency tree are required. The helper expects host tool wrappers, a local XNU SDK, and the `libpthread_kext`, crypto, and Firehose kernel static libraries. Those SDK/library inputs are not shipped here. The stock-compiler compatibility include used locally is provided as `build-support/xnu_clang_compat.h`; the prepared host wrapper force-includes it from `xnu-deps/hostinc/xnu_clang_compat.h`. The dependency/SDK bootstrap is not yet a complete reproducible one-command process.

```sh
bash tools/build_kernel.sh build/xnu /path/to/prepared/xnu-deps
```

Output: `build/xnu/BUILD/obj/DEVELOPMENT_ARM64_BCM2837/mach.development.bcm2837`. Build parameters retain the experimental ARM64/BCM2837 configuration used by the platform port; it does not imply Raspberry Pi hardware.

The local native SDK recovery used Apple libdispatch 1415.100.11 and AvailabilityVersions 115.4 source inputs. Preserve the source licenses when recreating those dependencies. No Apple SDK is bundled.

## macOS root

The trial used Apple macOS Ventura 13.4 / 22F66 restoration inputs, with a read-only RAM root and temporary data area. Standalone executables and the dyld shared cache needed non-PAC compatibility changes and signature-page processing. Full system resources were required beyond the recovery image. WindowServer used its software compositor, with experimental framework compatibility changes.

This repository does not currently turn a fresh Apple download into the tested desktop root end-to-end. The standalone helpers are only one part of that work. Do not interpret `mkapfs.py` as a macOS installer: it packs an already assembled tree into the port's read-only APFS image format.

## Host tests

`sh tests/run.sh` runs the pure coordinate/rotation cases under AddressSanitizer and UndefinedBehaviorSanitizer. It does not validate WindowServer behavior, touch alignment on the physical panel, thermal trip crossings, or full GPU desktop acceleration.
