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
/camera/image_raw
/run_slam/camera_pose
/tf
```

## First Validation

On Spark:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_live_demo_1440.sh
```

For a headless split, the next script should run only camera/SLAM/TF on Spark and not start RViz.

On the development workstation:

```bash
export ROS_DOMAIN_ID=15
source /opt/ros/jazzy/setup.bash
ros2 topic list
ros2 topic hz /run_slam/camera_pose
rviz2 -d /home/stvli/Desktop/where_is_my_key/x4_slam/config/rviz/stella_slam_demo.rviz
```

## Next Implementation Tasks

1. Add a Spark-side headless live script that starts bringup, SLAM, and `odom_to_tf.py`, but not RViz.
2. Mirror the RViz config and lightweight scripts to the development workstation workspace.
3. Check DDS discovery across `192.168.88.11` and `192.168.88.12` with `ROS_DOMAIN_ID=15`.
4. If multicast discovery is unreliable while moving, add a CycloneDDS/FastDDS peer configuration pinned to the robot LAN.
5. Decide whether `/camera/image_raw` is too heavy for Wi-Fi; if needed, visualize compressed image or only pose/TF remotely.
