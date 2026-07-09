#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BAG="${1:-$BASE_DIR/data/bags/smoke_test_002}"
RATE="${RATE:-0.2}"

if [[ $# -gt 0 ]]; then
  shift
fi

if [[ ! -f "$BAG/metadata.yaml" ]]; then
  echo "Bag metadata not found: $BAG/metadata.yaml" >&2
  exit 1
fi

set +u
source /opt/ros/jazzy/setup.bash
source "$BASE_DIR/ros2_ws/install/setup.bash"
set -u

exec ros2 bag play "$BAG" --clock --rate "$RATE" "$@"
