# Repository Organization

Date: 2026-07-09

## Purpose

This project is larger than one SLAM package. The top-level repository owns system integration, hardware topology, operating procedures, and cross-repository coordination.

## Repository Map

| Repository | Role | Local path | Status |
| --- | --- | --- | --- |
| `StvLi/Lite_Insta_Agilex_Slam` | Main orchestration repository | `/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam` | Active |
| `TeamLite-DeepCybo/lite_ros2` | Lite robot ROS2 control stack | not cloned here | Known, inactive |
| Agilex chassis repository | Chassis integration | pending | To be supplied |

## Current SLAM Code Location

The current Spark-side SLAM implementation snapshot is stored in:

```text
x4_slam
```

It was synchronized from:

```text
/home/deep/peize/where_is_my_key/x4_slam
```

The snapshot intentionally excludes:

- `data/` bags, maps, logs, and evaluation output
- `ros2_ws/build`, `ros2_ws/install`, `ros2_ws/log`
- `third_party/build`, `third_party/install`, `third_party/src`
- CameraSDK binaries
- `config/orb_vocab.fbow`
- local debug scratch files

## Upstream Usage

`Longxiaoze/360Vslam` was used as a reference upstream, not as the active
runtime package. It helped confirm the X4 + Stella VSLAM route and points to
the ROS2 camera driver path we actually adapted.

Current relationship:

| Repository | How it is used now |
| --- | --- |
| `Longxiaoze/360Vslam` | Reference upstream. Local mirror: `/home/deep/peize/where_is_my_key/ref_repo/src/360Vslam`, commit `2307480`. |
| `Longxiaoze/insta360_ros_driver` | Active camera driver base. Main-repo adapted copy: `/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/src/insta360_ros_driver`, commit `04cc799` before local changes. |
| `StvLi/360Vslam` | Not used as a project subrepository. It was considered briefly, then removed from the main repository structure. |

The code we have actually run is:

```text
Insta360 X4
  -> adapted insta360_ros_driver
  -> dual-fisheye decode
  -> equirectangular image
  -> stella_vslam_ros
  -> /run_slam/camera_pose
  -> odom_to_tf.py
```

We did not build or use `Longxiaoze/360Vslam/main.cpp` as the live SLAM
runtime.

## Why This Structure

- The main repository directly owns the active SLAM implementation.
- There is no artificial submodule boundary for code we maintain together.
- Heavy experiment artifacts stay local.
- Future robot-control and chassis repositories can be added without mixing concerns.

## Push Strategy

Because GitHub CLI authentication is currently invalid on the Spark machine, do not assume pushes will work from Spark until re-authenticated.

Recommended order:

1. Commit code and docs directly in `Lite_Insta_Agilex_Slam`.
2. Keep large local assets untracked.
3. Push `StvLi/Lite_Insta_Agilex_Slam` after GitHub authentication is fixed.

## Future Layout

Potential final layout:

```text
x4_slam/             # perception/SLAM
external/
  lite_ros2/          # robot body and arms, if later added
  agilex_ros2/        # chassis, if later added
docs/
  hardware_topology.md
  remote_visualization_plan.md
  network_and_dds.md
  bringup_sequences.md
```
