#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BAG_NAME="${1:-slam_acceptance_$(date +%Y%m%d_%H%M%S)}"
DURATION="${DURATION:-60}"

if [[ "$BAG_NAME" = /* ]]; then
  BAG_PATH="$BAG_NAME"
else
  BAG_PATH="$BASE_DIR/data/bags/$BAG_NAME"
fi

if [[ -e "$BAG_PATH" ]]; then
  echo "Bag path already exists: $BAG_PATH" >&2
  exit 1
fi

mkdir -p "$(dirname "$BAG_PATH")"

set +u
source /opt/ros/jazzy/setup.bash
source "$BASE_DIR/ros2_ws/install/setup.bash"
set -u

echo "Recording SLAM bag:"
echo "  output:   $BAG_PATH"
echo "  duration: $DURATION seconds"
echo
echo "Motion plan:"
echo "  0-5s    hold still"
echo "  5-20s   translate left/right around textured near objects"
echo "  20-40s  walk forward/backward slowly"
echo "  40-55s  make a small loop around a table/chair/box"
echo "  55-60s  hold still"
echo
echo "Avoid pure rotation. The camera position must move."
echo

exec timeout --signal=INT "$DURATION" ros2 bag record \
  --topics \
  /camera/image_raw \
  /imu/data \
  /imu/data_raw \
  /dual_fisheye/image/compressed \
  -o "$BAG_PATH"
