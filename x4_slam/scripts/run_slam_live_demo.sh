#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RVIZ_CONFIG="${RVIZ_CONFIG:-$BASE_DIR/config/rviz/stella_slam_demo.rviz}"
SLAM_LOG_LEVEL="${SLAM_LOG_LEVEL:-info}"
IMAGE_TOPIC="${IMAGE_TOPIC:-/camera/image_raw}"
STREAM_RESOLUTION="${STREAM_RESOLUTION:-RES_1920_960P30}"
STREAM_BITRATE_MBPS="${STREAM_BITRATE_MBPS:-6}"
DECODER_SKIP_FRAME="${DECODER_SKIP_FRAME:-0}"
EQUIRECTANGULAR_CONFIG="${EQUIRECTANGULAR_CONFIG:-}"
STAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${LOG_DIR:-$BASE_DIR/data/logs/live_demo_$STAMP}"
MAP_OUT="${MAP_OUT:-$BASE_DIR/data/maps/live_$STAMP.msg}"
EVAL_LOG_DIR="${EVAL_LOG_DIR:-$BASE_DIR/data/slam_eval/live_$STAMP}"
BRINGUP_TIMEOUT="${BRINGUP_TIMEOUT:-45}"

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
  echo "Stopping live demo..."
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

echo "Starting live Insta360 SLAM demo"
echo "Image topic: $IMAGE_TOPIC"
echo "Stream resolution: $STREAM_RESOLUTION"
echo "Stream bitrate: ${STREAM_BITRATE_MBPS} Mbps"
echo "Decoder skip frame: $DECODER_SKIP_FRAME"
if [[ -n "$EQUIRECTANGULAR_CONFIG" ]]; then
  echo "Equirectangular config: $EQUIRECTANGULAR_CONFIG"
fi
echo "RViz: $RVIZ_CONFIG"
echo "Logs: $LOG_DIR"
echo
echo "Camera should be connected in Android/SDK mode."
echo "Close RViz or press Ctrl-C here to stop everything."
echo

BRINGUP_ARGS=(
  stream_resolution:="$STREAM_RESOLUTION" \
  stream_bitrate_mbps:="$STREAM_BITRATE_MBPS" \
  decoder_skip_frame:="$DECODER_SKIP_FRAME"
)
if [[ -n "$EQUIRECTANGULAR_CONFIG" ]]; then
  BRINGUP_ARGS+=(equirectangular_config:="$EQUIRECTANGULAR_CONFIG")
fi

"$BASE_DIR/scripts/run_bringup.sh" "${BRINGUP_ARGS[@]}" \
  >"$LOG_DIR/bringup.log" 2>&1 &
PIDS+=("$!")

echo "Waiting for $IMAGE_TOPIC ..."
deadline=$((SECONDS + BRINGUP_TIMEOUT))
until ros2 topic list 2>/dev/null | grep -Fxq "$IMAGE_TOPIC"; do
  if (( SECONDS >= deadline )); then
    echo "Timed out waiting for $IMAGE_TOPIC after ${BRINGUP_TIMEOUT}s" >&2
    echo "See bringup log: $LOG_DIR/bringup.log" >&2
    exit 1
  fi
  sleep 1
done
echo "$IMAGE_TOPIC is available."

MAP_OUT="$MAP_OUT" \
EVAL_LOG_DIR="$EVAL_LOG_DIR" \
IMAGE_TOPIC="$IMAGE_TOPIC" \
"$BASE_DIR/scripts/run_slam.sh" --log-level "$SLAM_LOG_LEVEL" \
  >"$LOG_DIR/slam.log" 2>&1 &
PIDS+=("$!")

sleep 2

"$BASE_DIR/scripts/odom_to_tf.py" \
  >"$LOG_DIR/odom_to_tf.log" 2>&1 &
PIDS+=("$!")

sleep 1

rviz2 -d "$RVIZ_CONFIG" \
  >"$LOG_DIR/rviz.log" 2>&1 &
RVIZ_PID="$!"
PIDS+=("$RVIZ_PID")

while kill -0 "$RVIZ_PID" >/dev/null 2>&1; do
  sleep 2
done
