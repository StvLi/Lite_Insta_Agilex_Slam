# Remote Visualization Plan

Date: 2026-07-09

Goal: carry the Spark and Insta360 X4 on the robot while rendering RViz on the main development workstation.

## Split

Spark robot computer:

```text
deep@192.168.88.11
Runs: camera driver, equirectangular conversion, stella_vslam_ros, odom_to_tf
Display: none
ROS_DOMAIN_ID=15
```

Main development workstation:

```text
stvli@192.168.88.12
Runs: RViz and operator visualization
Display: yes
ROS_DOMAIN_ID=15
```

## Expected Topics

Spark publishes:

```text
/camera/image_raw
/dual_fisheye/image/compressed
/imu/data
/imu/data_raw
/run_slam/camera_pose
/tf
```

Development workstation subscribes:

```text
/run_slam/camera_pose
/tf
```

The Spark and development workstation use a bandwidth-limited bridge. Remote
RViz should not subscribe to `/camera/image_raw` by default. Normal remote
visualization is pose/TF-only; image display is an explicit debug mode.

## Next Engineering Step

Use the Spark-side headless script to start camera, SLAM, and TF without RViz:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_headless_1440.sh
```

Use the low-bandwidth RViz script on `stvli@192.168.88.12`:

```bash
/home/stvli/Desktop/where_is_my_key/src/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_remote_rviz_low_bandwidth.sh
```

If DDS multicast discovery is unreliable while moving, add explicit peer configuration for the robot LAN.
