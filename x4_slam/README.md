# x4_slam

Insta360 X4/X5 panoramic SLAM workspace for the ARM64 NVIDIA DGX Spark host.

Current hardware:

- Main dev/visualization workstation: `stvli@192.168.88.12`, x86_64, Ubuntu 24.04, ROS2 Jazzy
- Robot local computer: `deep@192.168.88.11`, NVIDIA DGX Spark, ARM64/AArch64, Ubuntu 24.04, ROS2 Jazzy
- Motion controller: `sunrise@192.168.88.10`, ARM, Ubuntu 22.04, ROS2 Humble
- ROS domain: `ROS_DOMAIN_ID=15` on all three machines
- Robot local memory: 128GB unified/shared memory
- Camera now: Insta360 X4 in Android/SDK mode
- Previous development stand-in: Insta360 X5

## Current State

The pipeline is working end to end:

```text
Insta360 CameraSDK
  -> H.264 dual-fisheye compressed stream
  -> ROS2 decoder
  -> equirectangular image
  -> stella_vslam_ros
  -> /run_slam/camera_pose
  -> odom_to_tf.py
  -> RViz map -> slam_camera
```

The best realtime profile found so far is `1440x720`: it improves smoothness over `1920x960` while keeping better tracking quality than `1024x512`.

## Main Commands

Link local SDK, vocabulary, and third-party build assets:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/link_local_assets.sh
```

Build:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/build_ros_ws.sh
```

Live SLAM, default fast profile `1920x960`:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo.sh
```

Live SLAM, current best candidate `1440x720`:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1440.sh
```

Live SLAM, low-latency boundary test `1024x512`:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1024.sh
```

Offline bag demo:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_bag_demo.sh \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/data/bags/parallax_texture_002
```

Record a new acceptance bag:

```bash
DURATION=60 /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/record_slam_bag.sh parallax_texture_next
```

## Profiles

| Script | SDK stream | Equirectangular | Stella config | Status |
| --- | --- | --- | --- | --- |
| `run_slam_live_demo.sh` | `RES_1920_960P30` | `1920x960` | `config/insta360X4_equirectangular.yaml` | clear, occasional stalls |
| `run_slam_live_demo_1440.sh` | `RES_1440_720P30` | `1440x720` | `config/insta360X4_equirectangular_1440.yaml` | current best candidate |
| `run_slam_live_demo_1024.sh` | `RES_1024_512P30` | `1024x512` | `config/insta360X4_equirectangular_1024.yaml` | faster, tracking quality worse |

## Layout

```text
config/          Stella configs, ORB vocab, RViz config
docs/            deployment notes, validation steps, troubleshooting
ros2_ws/src/     ROS2 source packages and local patches
scripts/         one-command build/run/record helpers
sdk/             symlink to official ARM64 CameraSDK
third_party/     local stella_vslam/g2o/FBoW source/build/install area
data/            local bags, maps, logs, eval output
```

Large local outputs and vendor assets are intentionally ignored by `.gitignore`: bags, maps, logs, build products, third-party installs, CameraSDK headers/binaries, and `orb_vocab.fbow`.

## Documentation

- Detailed implementation plan: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
- Hardware summary: [HARDWARE.md](HARDWARE.md)
- Hardware topology: [docs/hardware_topology.md](docs/hardware_topology.md)
- Remote visualization plan: [docs/remote_visualization_plan.md](docs/remote_visualization_plan.md)
- Current deployment status: [docs/deployment_status.md](docs/deployment_status.md)
- SLAM validation route: [docs/slam_validation.md](docs/slam_validation.md)
- Troubleshooting: [docs/troubleshooting.md](docs/troubleshooting.md)
- Top-level compliance notes: [../docs/open_source_compliance.md](../docs/open_source_compliance.md)

## Notes

- Use Android/SDK mode on the Insta360 camera, not UVC webcam mode.
- The current camera is X4. X5 was only the early development stand-in; keep calibration and camera-specific tuning explicit.
- `stella_vslam_ros` currently consumes image only. `/imu/data` is recorded for later VIO/fusion work.
