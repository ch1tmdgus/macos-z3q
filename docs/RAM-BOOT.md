# RAM boot trials

Use an already compatible **xnuloader** on the exact supported phone. This repository does not automate flashing Samsung boot/recovery partitions. Preserve your own verified device backups and recovery route separately.

Supply a locally built kernel, its exact SHA-256, and a prepared macOS RAM root. The host needs Python 3 and Android platform-tools (`fastboot`).

```sh
python3 tools/boot_ram.py --trial 1 \
  --kernel /path/to/mach.development.bcm2837 \
  --expected-kernel-sha256 YOUR_64_CHARACTER_SHA256 \
  --root /path/to/prepared-root.apfs \
  --seconds 0 --observe-seconds 120 \
  --graphics-fast --cpus 4 --factory-cpu-frequency --cpu-control \
  --touch-steps 7 --landscape --hid-input-scale 2
```

`--seconds 0` leaves macOS running after observation. A positive supported duration returns to the loader. `--gpu 1` additionally attempts Adreno initialization and requires matching firmware in the root; it does not enable the macOS Metal compositor. The 2x HID conversion is a trial-17 candidate, not a confirmed fix for all touch/cursor issues.

The runner checks the kernel digest before staging and uses `oem ramdisk`, `stage`, and `oem run`; it contains no partition-flash command. Logs and payloads go under ignored `runs/`. `--reuse-from-trial` is only meaningful while the same root remains resident after an OS return; reboot/power loss discards it.

To end a running test, `fastboot -s xnuloader oem stop` returns it to the USB loader. Poll `getvar result` until it says `returned`, then collect `get_staged` **before** other export commands if you need the final log. This does not restore or reboot Android.

`--capture-frame` requests a small native surface recording. Export it with `oem recording` only after return to the loader, then `get_staged recording.bin`. The decoder accepts RWREC003. Firmware screenshot commands read an old console buffer and are not proof of the active desktop.

Charging and battery management are not validated in this RAM profile; normal Android power behavior is not provided by these boot arguments.
