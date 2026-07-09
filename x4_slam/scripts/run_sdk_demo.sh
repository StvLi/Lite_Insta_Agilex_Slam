#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SDK="${SDK:-$BASE_DIR/sdk/CameraSDK}"

if [[ ! -x "$SDK/bin/CameraSDKTest" ]]; then
  echo "CameraSDKTest not found or not executable: $SDK/bin/CameraSDKTest" >&2
  exit 1
fi

export LD_LIBRARY_PATH="$SDK/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$SDK/bin"
exec ./CameraSDKTest "$@"
