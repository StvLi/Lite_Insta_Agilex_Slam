#!/usr/bin/env bash
set -euo pipefail

echo "== Insta360 USB devices =="
lsusb | grep -Ei 'insta|2e1a' || true

echo
echo "== USB topology =="
lsusb -t

echo
echo "== Video nodes =="
ls -l /dev/video* /dev/v4l/by-id/*Insta* 2>/dev/null || true

echo
echo "== USB device node permissions =="
while read -r line; do
  bus_num="$(awk '{print $2}' <<<"$line")"
  dev_num="$(awk '{print $4}' <<<"$line" | tr -d ':')"
  node="/dev/bus/usb/${bus_num}/${dev_num}"
  echo "$line"
  if [[ -e "$node" ]]; then
    ls -l "$node"
    python3 - "$node" <<'PY'
import os
import sys
node = sys.argv[1]
print(f"  readable={os.access(node, os.R_OK)} writable={os.access(node, os.W_OK)}")
PY
  fi
done < <(lsusb | grep -Ei 'insta|2e1a' || true)

echo
echo "Expected SDK mode: vendor-specific USB device, usually no /dev/video* node."
echo "If the USB node is not writable by this user, install the udev rule in docs/troubleshooting.md."
