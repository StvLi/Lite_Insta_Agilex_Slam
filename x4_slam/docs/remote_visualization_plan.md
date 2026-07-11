# Remote Visualization Plan

Date: 2026-07-09

Goal: carry the Spark and Insta360 X4 around on the robot while rendering SLAM visualization on the main development workstation.

## Target Split

Spark robot local computer:

```text
deep@192.168.88.11
Runs: Insta360 driver, equirectangular conversion, stella_vslam_ros, odom_to_tf
Display: none
ROS_DOMAIN_ID=15
```

Main development workstation:

```text
stvli@192.168.88.12
Runs: RViz, operator tools, optional bag record/inspection
Display: yes
ROS_DOMAIN_ID=15
Workspace: /home/stvli/Desktop/where_is_my_key
```

Motion controller:

```text
sunrise@192.168.88.10
Not in the current visualization path
ROS_DOMAIN_ID=15
```

## Expected ROS Graph

Spark publishes:

```text
/camera/image_raw
/dual_fisheye/image/compressed
/imu/data
/imu/data_raw
/run_slam/camera_pose
/tf                  # via odom_to_tf.py: map -> slam_camera
```

Development workstation subscribes:

```text
/run_slam/camera_pose
/tf
```

The Spark and development workstation are connected through a bandwidth-limited
bridge. Remote RViz must not subscribe to `/camera/image_raw` by default. Use the
low-bandwidth RViz config for normal operation; image display is an explicit
debug choice only.

## First Validation

On Spark:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_headless_1440.sh
```

On the development workstation:

```bash
/home/stvli/Desktop/where_is_my_key/src/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_remote_rviz_low_bandwidth.sh
```

This RViz config intentionally has no Image display. To verify that the
development workstation is not pulling video across the bridge, check
`ros2 topic info -v /camera/image_raw` on Spark and confirm there are no remote
subscribers.

## Next Implementation Tasks

1. Mirror the low-bandwidth RViz config and lightweight scripts to the development workstation workspace.
2. Check DDS discovery across `192.168.88.11` and `192.168.88.12` with `ROS_DOMAIN_ID=15`.
3. If multicast discovery is unreliable while moving, add a CycloneDDS/FastDDS peer configuration pinned to the robot LAN.
4. Add an optional low-resolution or compressed image relay only if operator video becomes necessary.
