# Stella VSLAM Validation

Date: 2026-07-06

This document is the short operational route from environment startup to a visible SLAM localization result.

## What Is Connected

- ROS2 workspace: `/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws`
- Stella ROS wrapper: `ros2_ws/src/stella_vslam_ros`
- Stella/FBoW/g2o local prefix: `third_party/install`
- ORB vocabulary: `config/orb_vocab.fbow`
- Equirectangular config: `config/insta360X4_equirectangular.yaml`
- Default image input: `/camera/image_raw`
- Default pose output: `/run_slam/camera_pose`
- Default live profile: `RES_1920_960P30` camera stream and `1920x960` equirectangular image

`run_slam.sh` exports the local library path, loads the vocabulary/config, disables TF publishing for now, and leaves pose validation on `/run_slam/camera_pose`.

## Build Check

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/build_ros_ws.sh
```

Expected:

- `insta360_ros_driver` builds.
- `stella_vslam_ros` builds.
- `ros2 pkg executables stella_vslam_ros` lists `run_slam`, `run_slam_offline`, and `system`.

## Offline Bag Acceptance

One-click temporary demo:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_bag_demo.sh
```

It uses `data/bags/parallax_texture_001`, starts Stella VSLAM, replays the bag at `RATE=0.2` in a loop, and opens RViz with `config/rviz/stella_slam_demo.rviz`.

The demo also runs `scripts/odom_to_tf.py`, which converts `/run_slam/camera_pose` into a dynamic `map -> slam_camera` TF for RViz. The orange odometry arrow and the `slam_camera` TF should move together.

To use another bag:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_bag_demo.sh \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/slam_acceptance_001
```

Close RViz or press `Ctrl-C` in the terminal to stop the demo and clean up child processes.

Manual three-terminal route:

Terminal 1:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

Terminal 2:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/play_smoke_bag.sh
```

Terminal 3:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_slam_outputs.sh
```

Success criteria:

- `/run_slam/camera_pose` publishes `nav_msgs/msg/Odometry`.
- `ros2 topic hz /run_slam/camera_pose` reports a nonzero rate while the bag is playing.
- The SLAM terminal logs leave initialization and continue tracking.
- `data/maps/stella_<timestamp>.msg` is larger than a header-only empty map.
- `data/slam_eval/stella_<timestamp>/frame_trajectory.txt` or `keyframe_trajectory.txt` contains trajectory rows.

Current status with `data/bags/smoke_test_002`: transport and SLAM startup work, but the bag did not initialize a valid monocular map. It produced no pose sample and saved an empty map. Treat this bag as an image-pipeline smoke test, not as final SLAM acceptance.

## Live Camera Acceptance

One-click live demo:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo.sh
```

It starts the Insta360 bringup, waits for `/camera/image_raw`, starts Stella VSLAM, converts `/run_slam/camera_pose` into `map -> slam_camera` TF, and opens RViz. Use this route for direct camera experiments.

The default live demo is the fast route:

```text
STREAM_RESOLUTION=RES_1920_960P30
STREAM_BITRATE_MBPS=6
DECODER_SKIP_FRAME=0
```

Slightly lower balanced route:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1440.sh
```

This uses:

```text
SDK stream: RES_1440_720P30
Equirectangular output: 1440x720
Stella config: config/insta360X4_equirectangular_1440.yaml
Bitrate: 5 Mbps
```

Lower low-latency route:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1024.sh
```

This uses:

```text
SDK stream: RES_1024_512P30
Equirectangular output: 1024x512
Stella config: config/insta360X4_equirectangular_1024.yaml
Bitrate: 4 Mbps
```

Optional comparison runs:

```bash
STREAM_RESOLUTION=RES_2560_1280P30 STREAM_BITRATE_MBPS=8 \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo.sh
```

High-resolution live comparison:

```bash
STREAM_RESOLUTION=RES_3840_1920P30 \
STREAM_BITRATE_MBPS=10 \
EQUIRECTANGULAR_CONFIG=/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/install/insta360_ros_driver/share/insta360_ros_driver/config/equirectangular_3840.yaml \
CONFIG=/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/config/insta360X4_equirectangular_3840.yaml \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo.sh
```

For old 3840x1920 bags or high-resolution comparison, use:

```bash
CONFIG=/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/config/insta360X4_equirectangular_3840.yaml \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_bag_demo.sh \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/parallax_texture_002
```

Terminal 1:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh
```

Terminal 2:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

Terminal 3:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_slam_outputs.sh
```

If the image topic changes, override it:

```bash
IMAGE_TOPIC=/your/equirectangular/topic /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

## Recording A Better Acceptance Bag

One-command recorder after `run_bringup.sh` is running:

```bash
DURATION=60 /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/record_slam_bag.sh parallax_texture_002
```

The script records `/camera/image_raw`, `/imu/data`, `/imu/data_raw`, and `/dual_fisheye/image/compressed`, then stops automatically after `DURATION` seconds.

Record at least 45-90 seconds:

```bash
ros2 bag record \
  --topics \
  /camera/image_raw \
  /imu/data \
  /tf \
  -o /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/slam_acceptance_001
```

Motion guidance:

- Use a textured scene with clear corners, furniture, shelves, posters, or doors.
- Include slow sideways translation, forward/backward translation, and gentle turns.
- Avoid pure rotation in place; monocular initialization needs parallax.
- Avoid pointing mostly at blank walls, ceiling, floor, mirrors, or strong glare.
- Keep the first 5-10 seconds slow and stable, then move through the scene.

Replay it slowly:

```bash
RATE=0.2 /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/play_smoke_bag.sh \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/slam_acceptance_001
```

## Useful Diagnostics

```bash
ros2 topic hz /camera/image_raw
ros2 topic echo --once /camera/image_raw/header
ros2 topic echo --once /run_slam/camera_pose
ros2 bag info /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/slam_acceptance_001
```

If `/run_slam/camera_pose` stays silent while images are flowing, check the SLAM terminal. Repeated initialization attempts usually mean the sequence lacks enough parallax or texture, or the equirectangular parameters need tuning.
