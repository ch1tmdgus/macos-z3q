#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT HUP INT TERM
for test in rotation hid_coordinates; do
 "${CC:-clang}" -fsanitize=address,undefined -g -Ikernel-overlay/iokit/Kernel "tests/$test.c" -o "$tmp/$test"
 "$tmp/$test"
done
