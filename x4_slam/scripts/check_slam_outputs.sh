#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
POSE_TOPIC="${POSE_TOPIC:-/run_slam/camera_pose}"
KEYFRAMES_TOPIC="${KEYFRAMES_TOPIC:-/run_slam/keyframes}"
TIMEOUT="${TIMEOUT:-10}"

set +u
source /opt/ros/jazzy/setup.bash
source "$BASE_DIR/ros2_ws/install/setup.bash"
set -u

echo "== SLAM topics =="
ros2 topic list -t | grep -E 'run_slam|camera_pose|keyframes|/tf' || true

echo
echo "== One pose sample =="
timeout "$TIMEOUT" ros2 topic echo --once "$POSE_TOPIC" || true

echo
echo "== Pose rate =="
timeout "$TIMEOUT" ros2 topic hz "$POSE_TOPIC" || true

echo
echo "== Keyframes topic =="
ros2 topic info "$KEYFRAMES_TOPIC" || true
