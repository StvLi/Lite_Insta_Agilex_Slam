#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RVIZ_CONFIG="${RVIZ_CONFIG:-$BASE_DIR/config/rviz/stella_slam_remote_low_bandwidth.rviz}"

export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-15}"
export ROS_AUTOMATIC_DISCOVERY_RANGE="${ROS_AUTOMATIC_DISCOVERY_RANGE:-SUBNET}"
unset ROS_LOCALHOST_ONLY

if [[ ! -f "$RVIZ_CONFIG" ]]; then
  echo "RViz config not found: $RVIZ_CONFIG" >&2
  exit 1
fi
if ! command -v rviz2 >/dev/null 2>&1; then
  echo "rviz2 is not available. Install ROS Jazzy RViz first." >&2
  exit 1
fi

set +u
source /opt/ros/jazzy/setup.bash
if [[ -f "$BASE_DIR/ros2_ws/install/setup.bash" ]]; then
  source "$BASE_DIR/ros2_ws/install/setup.bash"
fi
set -u

echo "Starting low-bandwidth remote RViz"
echo "ROS_DOMAIN_ID: $ROS_DOMAIN_ID"
echo "RViz config: $RVIZ_CONFIG"
echo "Subscribed visualization topics: /run_slam/camera_pose and /tf"
echo "No image display is enabled by default."

exec rviz2 -d "$RVIZ_CONFIG"
