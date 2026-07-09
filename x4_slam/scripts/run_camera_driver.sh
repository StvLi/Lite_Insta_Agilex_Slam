#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SETUP="$BASE_DIR/ros2_ws/install/setup.bash"
THIRD_PARTY="$BASE_DIR/third_party/install"

if [[ ! -f "$SETUP" ]]; then
  echo "ROS workspace is not built yet: $SETUP" >&2
  echo "Run: $BASE_DIR/scripts/build_ros_ws.sh" >&2
  exit 1
fi

export LD_LIBRARY_PATH="$THIRD_PARTY/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

set +u
source /opt/ros/jazzy/setup.bash
source "$SETUP"
set -u
exec ros2 run insta360_ros_driver insta360_ros_driver "$@"
