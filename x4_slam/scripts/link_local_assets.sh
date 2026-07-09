#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_ROOT="$(cd "$BASE_DIR/.." && pwd)"

DEFAULT_REF_ROOT="/home/deep/peize/where_is_my_key"
DEFAULT_SDK="$DEFAULT_REF_ROOT/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1/CameraSDK-20251105_140609-2.1.1-gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu"
DEFAULT_LEGACY_WS="$DEFAULT_REF_ROOT/x4_slam"

SDK_PATH="${SDK_PATH:-$DEFAULT_SDK}"
VOCAB_PATH="${VOCAB_PATH:-$DEFAULT_LEGACY_WS/config/orb_vocab.fbow}"
THIRD_PARTY_INSTALL="${THIRD_PARTY_INSTALL:-$DEFAULT_LEGACY_WS/third_party/install}"

require_path() {
  local path="$1"
  local label="$2"
  if [[ ! -e "$path" ]]; then
    echo "$label not found: $path" >&2
    exit 1
  fi
}

require_path "$SDK_PATH/include/camera" "CameraSDK camera headers"
require_path "$SDK_PATH/include/stream" "CameraSDK stream headers"
require_path "$SDK_PATH/lib/libCameraSDK.so" "CameraSDK shared library"
require_path "$VOCAB_PATH" "ORB vocabulary"
require_path "$THIRD_PARTY_INSTALL" "third-party install prefix"

mkdir -p \
  "$BASE_DIR/config" \
  "$BASE_DIR/sdk" \
  "$BASE_DIR/third_party" \
  "$BASE_DIR/ros2_ws/src/insta360_ros_driver/include" \
  "$BASE_DIR/ros2_ws/src/insta360_ros_driver/lib"

ln -sfn "$SDK_PATH" "$BASE_DIR/sdk/CameraSDK"
ln -sfn "$SDK_PATH/include/camera" "$BASE_DIR/ros2_ws/src/insta360_ros_driver/include/camera"
ln -sfn "$SDK_PATH/include/stream" "$BASE_DIR/ros2_ws/src/insta360_ros_driver/include/stream"
ln -sfn "$SDK_PATH/lib/libCameraSDK.so" "$BASE_DIR/ros2_ws/src/insta360_ros_driver/lib/libCameraSDK.so"
ln -sfn "$VOCAB_PATH" "$BASE_DIR/config/orb_vocab.fbow"
ln -sfn "$THIRD_PARTY_INSTALL" "$BASE_DIR/third_party/install"

echo "Linked local assets for $PROJECT_ROOT/x4_slam"
