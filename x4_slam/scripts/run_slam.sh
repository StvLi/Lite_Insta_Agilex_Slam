#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SETUP="$BASE_DIR/ros2_ws/install/setup.bash"
THIRD_PARTY="$BASE_DIR/third_party/install"
STAMP="$(date +%Y%m%d_%H%M%S)"

VOCAB="${VOCAB:-$BASE_DIR/config/orb_vocab.fbow}"
CONFIG="${CONFIG:-$BASE_DIR/config/insta360X4_equirectangular.yaml}"
IMAGE_TOPIC="${IMAGE_TOPIC:-/camera/image_raw}"
MAP_OUT="${MAP_OUT:-$BASE_DIR/data/maps/stella_${STAMP}.msg}"
EVAL_LOG_DIR="${EVAL_LOG_DIR:-$BASE_DIR/data/slam_eval/stella_${STAMP}}"

if [[ ! -f "$SETUP" ]]; then
  echo "ROS workspace is not built yet: $SETUP" >&2
  echo "Run: $BASE_DIR/scripts/build_ros_ws.sh" >&2
  exit 1
fi
if [[ ! -s "$VOCAB" ]]; then
  echo "ORB vocabulary not found or empty: $VOCAB" >&2
  exit 1
fi
if [[ ! -f "$CONFIG" ]]; then
  echo "SLAM config not found: $CONFIG" >&2
  exit 1
fi

mkdir -p "$(dirname "$MAP_OUT")" "$EVAL_LOG_DIR"

export LD_LIBRARY_PATH="$THIRD_PARTY/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export CMAKE_PREFIX_PATH="$THIRD_PARTY${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"

set +u
source /opt/ros/jazzy/setup.bash
source "$SETUP"
set -u

exec ros2 run stella_vslam_ros run_slam \
  -v "$VOCAB" \
  -c "$CONFIG" \
  --viewer none \
  --map-db-out "$MAP_OUT" \
  --eval-log-dir "$EVAL_LOG_DIR" \
  "$@" \
  --ros-args \
  -r camera/image_raw:="$IMAGE_TOPIC" \
  -p encoding:=bgr8 \
  -p publish_tf:=false \
  -p publish_keyframes:=true
