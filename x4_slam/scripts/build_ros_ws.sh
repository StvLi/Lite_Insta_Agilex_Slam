#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WS="$BASE_DIR/ros2_ws"
THIRD_PARTY="$BASE_DIR/third_party/install"

export CMAKE_PREFIX_PATH="$THIRD_PARTY${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export LD_LIBRARY_PATH="$THIRD_PARTY/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

set +u
source /opt/ros/jazzy/setup.bash
set -u
cd "$WS"
exec colcon build --symlink-install --event-handlers console_direct+ "$@"
