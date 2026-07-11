#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
STAMP="$(date +%Y%m%d_%H%M%S)"

STREAM_RESOLUTION="${STREAM_RESOLUTION:-RES_1440_720P30}"
STREAM_BITRATE_MBPS="${STREAM_BITRATE_MBPS:-5}"
DECODER_SKIP_FRAME="${DECODER_SKIP_FRAME:-0}"
EQUIRECTANGULAR_CONFIG="${EQUIRECTANGULAR_CONFIG:-$BASE_DIR/ros2_ws/src/insta360_ros_driver/config/equirectangular_1440.yaml}"
CONFIG="${CONFIG:-$BASE_DIR/config/insta360X4_equirectangular_1440.yaml}"
IMAGE_TOPIC="${IMAGE_TOPIC:-/camera/image_raw}"
LOG_DIR="${LOG_DIR:-$BASE_DIR/data/logs/slam_headless_${STAMP}_1440}"
MAP_OUT="${MAP_OUT:-$BASE_DIR/data/maps/slam_headless_${STAMP}_1440.msg}"
EVAL_LOG_DIR="${EVAL_LOG_DIR:-$BASE_DIR/data/slam_eval/slam_headless_${STAMP}_1440}"
BRINGUP_TIMEOUT="${BRINGUP_TIMEOUT:-45}"
SLAM_TIMEOUT="${SLAM_TIMEOUT:-90}"
LOCK_FILE="$BASE_DIR/data/logs/run_slam_headless.lock"
TF_REPUBLISH_HZ="${TF_REPUBLISH_HZ:-20}"
TF_MAX_POSE_AGE_SEC="${TF_MAX_POSE_AGE_SEC:-2}"
TF_STAMP_MODE="${TF_STAMP_MODE:-now}"
TF_SOURCE_CHILD_FRAME="${TF_SOURCE_CHILD_FRAME:-camera_frame}"

mkdir -p "$LOG_DIR" "$(dirname "$MAP_OUT")" "$EVAL_LOG_DIR"
exec 9>"$LOCK_FILE"
if ! flock -n 9; then
  echo "Another headless SLAM session is already running." >&2
  echo "Stop it first, then rerun this script." >&2
  exit 1
fi
printf '%s\n' "$LOG_DIR" > "$BASE_DIR/data/logs/latest_slam_headless_dir.txt"

export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-15}"
export ROS_AUTOMATIC_DISCOVERY_RANGE="${ROS_AUTOMATIC_DISCOVERY_RANGE:-SUBNET}"
unset ROS_LOCALHOST_ONLY

set +u
source /opt/ros/jazzy/setup.bash
source "$BASE_DIR/ros2_ws/install/setup.bash"
set -u

PIDS=()
cleanup() {
  trap - EXIT INT TERM
  echo
  echo "Stopping headless SLAM..."
  for pid in "${PIDS[@]:-}"; do
    if kill -0 "$pid" >/dev/null 2>&1; then
      kill "$pid" >/dev/null 2>&1 || true
    fi
  done
  wait >/dev/null 2>&1 || true
  echo "Logs: $LOG_DIR"
  echo "Map out: $MAP_OUT"
  echo "Eval dir: $EVAL_LOG_DIR"
}
trap cleanup EXIT INT TERM

echo "Starting headless Insta360 SLAM"
echo "ROS_DOMAIN_ID: $ROS_DOMAIN_ID"
echo "Stream: $STREAM_RESOLUTION, ${STREAM_BITRATE_MBPS} Mbps"
echo "Image topic for SLAM: $IMAGE_TOPIC"
echo "TF republish: ${TF_REPUBLISH_HZ} Hz, max pose age ${TF_MAX_POSE_AGE_SEC}s, stamp ${TF_STAMP_MODE}"
echo "Source-stamped TF: map -> ${TF_SOURCE_CHILD_FRAME}"
echo "Logs: $LOG_DIR"
echo

"$BASE_DIR/scripts/run_bringup.sh" \
  decoder:=true \
  equirectangular:=true \
  imu_filter:=true \
  stream_resolution:="$STREAM_RESOLUTION" \
  stream_bitrate_mbps:="$STREAM_BITRATE_MBPS" \
  decoder_skip_frame:="$DECODER_SKIP_FRAME" \
  equirectangular_config:="$EQUIRECTANGULAR_CONFIG" \
  >"$LOG_DIR/bringup.log" 2>&1 &
PIDS+=("$!")
echo "${PIDS[-1]}" > "$LOG_DIR/bringup.pid"

echo "Waiting for $IMAGE_TOPIC ..."
deadline=$((SECONDS + BRINGUP_TIMEOUT))
until ros2 topic list 2>/dev/null | grep -Fxq "$IMAGE_TOPIC"; do
  if (( SECONDS >= deadline )); then
    echo "Timed out waiting for $IMAGE_TOPIC after ${BRINGUP_TIMEOUT}s" >&2
    echo "See: $LOG_DIR/bringup.log" >&2
    exit 1
  fi
  sleep 1
done
echo "$IMAGE_TOPIC is available."

CONFIG="$CONFIG" \
IMAGE_TOPIC="$IMAGE_TOPIC" \
MAP_OUT="$MAP_OUT" \
EVAL_LOG_DIR="$EVAL_LOG_DIR" \
"$BASE_DIR/scripts/run_slam.sh" \
  >"$LOG_DIR/slam.log" 2>&1 &
PIDS+=("$!")
echo "${PIDS[-1]}" > "$LOG_DIR/slam.pid"

echo "Waiting for /run_slam/camera_pose ..."
deadline=$((SECONDS + SLAM_TIMEOUT))
until ros2 topic list 2>/dev/null | grep -Fxq /run_slam/camera_pose; do
  if (( SECONDS >= deadline )); then
    echo "Timed out waiting for /run_slam/camera_pose after ${SLAM_TIMEOUT}s" >&2
    echo "See: $LOG_DIR/slam.log" >&2
    exit 1
  fi
  sleep 1
done
echo "/run_slam/camera_pose is available."

TF_REPUBLISH_HZ="$TF_REPUBLISH_HZ" \
TF_MAX_POSE_AGE_SEC="$TF_MAX_POSE_AGE_SEC" \
TF_STAMP_MODE="$TF_STAMP_MODE" \
TF_SOURCE_CHILD_FRAME="$TF_SOURCE_CHILD_FRAME" \
"$BASE_DIR/scripts/odom_to_tf.py" \
  >"$LOG_DIR/odom_to_tf.log" 2>&1 &
PIDS+=("$!")
echo "${PIDS[-1]}" > "$LOG_DIR/odom_to_tf.pid"

echo
echo "Headless SLAM is running."
echo "RViz fixed frame: map"
echo "Pose topic: /run_slam/camera_pose"
echo "TF: map -> slam_camera"
echo "Source-stamped camera TF: map -> ${TF_SOURCE_CHILD_FRAME}"
echo "Press Ctrl-C here to stop."
echo

wait -n "${PIDS[@]}"
