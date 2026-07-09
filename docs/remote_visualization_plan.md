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
/camera/image_raw
/run_slam/camera_pose
/tf
```

## Next Engineering Step

Add a Spark-side headless script that starts camera, SLAM, and TF without RViz. RViz should run on `stvli@192.168.88.12`.

If DDS multicast discovery is unreliable while moving, add explicit peer configuration for the robot LAN.
