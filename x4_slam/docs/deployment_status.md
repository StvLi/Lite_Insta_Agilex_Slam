# Deployment Status

Date: 2026-07-06

## Host

- Current robot local machine: NVIDIA DGX Spark
- SSH identity: `deep@192.168.88.11`
- CPU architecture: ARM64 / AArch64
- OS: Ubuntu 24.04.3 LTS
- Memory: 128GB unified/shared memory
- ROS: Jazzy, installed in the host environment outside conda
- Display: no, accessed over SSH
- ROS domain: `ROS_DOMAIN_ID=15`

Related machines:

- Main development workstation: `stvli@192.168.88.12`, x86_64, Ubuntu 24.04, ROS2 Jazzy, workspace `/home/stvli/Desktop/where_is_my_key`
- Motion-domain controller: `sunrise@192.168.88.10`, ARM, Ubuntu 22.04, ROS2 Humble with vendor optimizations

See `docs/hardware_topology.md` for the current multi-machine layout.

## Camera

- Current camera: Insta360 X4
- Previous development stand-in: Insta360 X5
- Current USB SDK mode: Android mode
- USB ID historically seen with X5: `2e1a:0002 Arashi Vision Insta360 X5`
- Current SDK check: re-run after connecting X4

Keep camera-specific parameters and calibration replaceable while validating the X4.

## SDK

The active SDK symlink is:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/sdk/CameraSDK
```

It points to the official ARM64 CameraSDK:

```text
/home/deep/peize/where_is_my_key/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1/CameraSDK-20251105_140609-2.1.1-gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu
```

Avoid the x86_64 CameraSDK and the amd64-only MediaSDK package on this host.

## USB Permission

The following udev rule has been installed:

```text
SUBSYSTEM=="usb", ATTR{idVendor}=="2e1a", MODE="0666", TAG+="uaccess"
```

Path:

```text
/etc/udev/rules.d/99-insta360.rules
```

This lets the SDK access the libusb device without running the ROS node as root.

## Community Driver Route

The active ROS2 driver is the community driver:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/src/insta360_ros_driver
```

Source:

```text
Longxiaoze/insta360_ros_driver, branch humble, commit 04cc799
```

Local changes made for this machine:

- Copied official ARM64 SDK headers and `libCameraSDK.so` into the driver.
- Added CMake RPATH/install handling so executables can find `libCameraSDK.so`.
- Updated `cv_bridge` include paths for ROS2 Jazzy.
- Updated FFmpeg codec pointer type for the installed FFmpeg headers.
- Updated SDK 2.1.1 time sync call signature.
- Replaced unavailable `GetCameraType()` usage with `DeviceDescriptor` camera type.
- Configured the X4/X5 live-stream sequence with `VIDEO_LIVEVIEW`, `FUNCTION_MODE_LIVE_STREAM`, configurable resolution, gyro enabled, audio disabled.
- Changed equirectangular output-size logging from every frame to once.

## Build And Run

Build:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/build_ros_ws.sh
```

Run only the SDK-backed camera driver:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_camera_driver.sh
```

Run full bringup with decoder, equirectangular projection, and Madgwick IMU filter:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh
```

The default live route is now the fast profile:

```text
SDK stream: RES_1920_960P30
Equirectangular output: 1920x960
Stella config: config/insta360X4_equirectangular.yaml
```

A lower-latency comparison profile is available:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1440.sh
```

It uses `RES_1440_720P30`, `1440x720` equirectangular output, and `config/insta360X4_equirectangular_1440.yaml`.

An even lower-latency profile is also available:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1024.sh
```

It uses `RES_1024_512P30`, `1024x512` equirectangular output, and `config/insta360X4_equirectangular_1024.yaml`. Treat this as a speed/quality boundary test.

Optional launch arguments:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh equirectangular:=false imu_filter:=false
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh stream_resolution:=RES_2560_1280P30 stream_bitrate_mbps:=8
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh stream_resolution:=RES_3840_1920P30 stream_bitrate_mbps:=10 equirectangular_config:=/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/install/insta360_ros_driver/share/insta360_ros_driver/config/equirectangular_3840.yaml
```

## Verified Topics

Default bringup currently publishes:

```text
/dual_fisheye/image/compressed  sensor_msgs/msg/CompressedImage
/dual_fisheye/image             sensor_msgs/msg/Image
/camera/image_raw               sensor_msgs/msg/Image
/imu/data_raw                   sensor_msgs/msg/Imu
/imu/data                       sensor_msgs/msg/Imu
/tf                             tf2_msgs/msg/TFMessage
```

Measured on 2026-07-06 with the earlier X5 stand-in in Android mode:

```text
/dual_fisheye/image/compressed  about 29.5 Hz
/dual_fisheye/image             about 15.9 Hz, 3840x1920, bgr8
/camera/image_raw               about 12.8 Hz, 3840x1920, bgr8
/imu/data_raw                   about 52 Hz
/imu/data                       about 52 Hz
```

The first H.264 decoder warnings about missing PPS can appear when the decoder joins mid-stream. The stream continues after keyframe recovery.

## Stella VSLAM Route

`stella_vslam_ros` is now connected into the same ROS2 workspace:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/src/stella_vslam_ros
```

The locally built Stella VSLAM dependency prefix is:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/third_party/install
```

This prefix contains the ARM64 builds of `stella_vslam`, `FBoW`, and `g2o`. Use the project scripts instead of invoking the executable directly, because the scripts export the local library path before launching ROS.

ORB vocabulary:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/config/orb_vocab.fbow
```

Current fast equirectangular config:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/config/insta360X4_equirectangular.yaml
```

The config is set for the default fast `/camera/image_raw` stream:

```text
model: equirectangular
cols: 1920
rows: 960
color_order: BGR
```

The previous 3840x1920 Stella config is preserved at:

```text
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/config/insta360X4_equirectangular_3840.yaml
```

Build the full workspace:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/build_ros_ws.sh
```

Run SLAM:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

By default it subscribes to `/camera/image_raw` and publishes:

```text
/run_slam/camera_pose
/run_slam/keyframes
/run_slam/keyframes_2d
```

To use another image topic:

```bash
IMAGE_TOPIC=/some/equirectangular/image /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

Replay the current smoke bag slowly:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/play_smoke_bag.sh
```

The default replay rate is `0.2` because `3840x1920` raw `bgr8` frames are expensive to process. Override with `RATE=1.0` only when the pipeline can keep up.

Check SLAM outputs:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_slam_outputs.sh
```

Current smoke result on `data/bags/smoke_test_002`: the wiring works and Stella receives frames, but this short bag did not initialize a valid monocular map. Shutdown produced an empty map and no `/run_slam/camera_pose` sample. For the next acceptance bag, record a longer, textured sequence with deliberate translational motion, not only rotation.

## Current Risks And Next Steps

- The equirectangular calibration parameters are still inherited from the community project and must be checked with the target X4.
- Raw `bgr8` 3840x1920 images are high bandwidth. The default live route has been reduced to 1920x960 for speed; use 2560x1280 or 3840x1920 only for comparison runs.
- The current Stella VSLAM route is monocular visual SLAM. It does not consume `/imu/data`; IMU is still useful for later VIO or downstream fusion.
- The current `smoke_test_002` bag validates image transport but not final tracking. It needs a better motion sequence before it can be used as a localization acceptance bag.
- When the X4 arrives, repeat SDK demo, ROS bringup, topic size/rate checks, and calibration.
