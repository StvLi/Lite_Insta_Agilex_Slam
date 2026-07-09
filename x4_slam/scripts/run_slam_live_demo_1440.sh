#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export STREAM_RESOLUTION="${STREAM_RESOLUTION:-RES_1440_720P30}"
export STREAM_BITRATE_MBPS="${STREAM_BITRATE_MBPS:-5}"
export DECODER_SKIP_FRAME="${DECODER_SKIP_FRAME:-0}"
export EQUIRECTANGULAR_CONFIG="${EQUIRECTANGULAR_CONFIG:-$BASE_DIR/ros2_ws/src/insta360_ros_driver/config/equirectangular_1440.yaml}"
export CONFIG="${CONFIG:-$BASE_DIR/config/insta360X4_equirectangular_1440.yaml}"

exec "$BASE_DIR/scripts/run_slam_live_demo.sh" "$@"
