#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BAG="$BASE_DIR/data/bags/parallax_texture_001"
BAG="${1:-$DEFAULT_BAG}"
RATE="${RATE:-0.2}"
LOOP="${LOOP:-true}"
SLAM_LOG_LEVEL="${SLAM_LOG_LEVEL:-info}"
RVIZ_CONFIG="${RVIZ_CONFIG:-$BASE_DIR/config/rviz/stella_slam_demo.rviz}"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${LOG_DIR:-$BASE_DIR/data/logs/demo_$STAMP}"
MAP_OUT="${MAP_OUT:-$BASE_DIR/data/maps/demo_$STAMP.msg}"
EVAL_LOG_DIR="${EVAL_LOG_DIR:-$BASE_DIR/data/slam_eval/demo_$STAMP}"
TF_REPUBLISH_HZ="${TF_REPUBLISH_HZ:-20}"
TF_MAX_POSE_AGE_SEC="${TF_MAX_POSE_AGE_SEC:-2}"
TF_STAMP_MODE="${TF_STAMP_MODE:-source}"
TF_SOURCE_CHILD_FRAME="${TF_SOURCE_CHILD_FRAME:-camera_frame}"

if [[ ! -f "$BAG/metadata.yaml" ]]; then
  echo "Bag metadata not found: $BAG/metadata.yaml" >&2
  exit 1
fi
if [[ ! -f "$RVIZ_CONFIG" ]]; then
  echo "RViz config not found: $RVIZ_CONFIG" >&2
  exit 1
fi
if ! command -v rviz2 >/dev/null 2>&1; then
  echo "rviz2 is not available. Install ROS Jazzy RViz first." >&2
  exit 1
fi

mkdir -p "$LOG_DIR" "$(dirname "$MAP_OUT")" "$EVAL_LOG_DIR"

set +u
source /opt/ros/jazzy/setup.bash
source "$BASE_DIR/ros2_ws/install/setup.bash"
set -u

PIDS=()
cleanup() {
  trap - EXIT INT TERM
  echo
  echo "Stopping demo..."
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

echo "Starting one-click SLAM demo"
echo "Bag: $BAG"
echo "Rate: $RATE"
echo "Loop: $LOOP"
echo "RViz: $RVIZ_CONFIG"
echo "TF republish: ${TF_REPUBLISH_HZ} Hz, max pose age ${TF_MAX_POSE_AGE_SEC}s, stamp ${TF_STAMP_MODE}"
echo "Source-stamped TF: map -> ${TF_SOURCE_CHILD_FRAME}"
echo "Logs: $LOG_DIR"
echo
echo "Close RViz or press Ctrl-C here to stop everything."
echo

MAP_OUT="$MAP_OUT" \
EVAL_LOG_DIR="$EVAL_LOG_DIR" \
"$BASE_DIR/scripts/run_slam.sh" --log-level "$SLAM_LOG_LEVEL" \
  >"$LOG_DIR/slam.log" 2>&1 &
PIDS+=("$!")

sleep 4

TF_REPUBLISH_HZ="$TF_REPUBLISH_HZ" \
TF_MAX_POSE_AGE_SEC="$TF_MAX_POSE_AGE_SEC" \
TF_STAMP_MODE="$TF_STAMP_MODE" \
TF_SOURCE_CHILD_FRAME="$TF_SOURCE_CHILD_FRAME" \
"$BASE_DIR/scripts/odom_to_tf.py" \
  >"$LOG_DIR/odom_to_tf.log" 2>&1 &
PIDS+=("$!")

sleep 1

rviz2 -d "$RVIZ_CONFIG" --ros-args -p use_sim_time:=true \
  >"$LOG_DIR/rviz.log" 2>&1 &
RVIZ_PID="$!"
PIDS+=("$RVIZ_PID")

sleep 2

PLAY_ARGS=()
if [[ "$LOOP" == "true" || "$LOOP" == "1" || "$LOOP" == "yes" ]]; then
  PLAY_ARGS+=(--loop)
fi

RATE="$RATE" "$BASE_DIR/scripts/play_smoke_bag.sh" "$BAG" "${PLAY_ARGS[@]}" \
  >"$LOG_DIR/bag_play.log" 2>&1 &
PIDS+=("$!")

while kill -0 "$RVIZ_PID" >/dev/null 2>&1; do
  sleep 2
done
