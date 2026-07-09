# Lite Insta Agilex Slam

Top-level engineering repository for the Lite robot perception/navigation stack.

This repository is the project orchestrator and the home for the current Insta360 X4 SLAM implementation. It records hardware topology, operating conventions, and the source code needed to maintain the Spark-side SLAM pipeline.

## Current Scope

Active now:

- Insta360 X4 panoramic visual SLAM on the Spark robot computer.
- Remote visualization from the main development workstation.
- ROS2 network split with `ROS_DOMAIN_ID=15`.

Known but not active yet:

- Lite dual-arm humanoid ROS control stack.
- Agilex chassis stack.
- D-Robotics/Horizon X5 motion-domain controller integration.

## Repository Layout

```text
Lite_Insta_Agilex_Slam/
  README.md
  docs/
    repository_organization.md
    hardware_topology.md
    remote_visualization_plan.md
    open_source_compliance.md
  x4_slam/                 # Current Spark-side Insta360 X4 SLAM workspace
    config/
    docs/
    ros2_ws/src/
    scripts/
  vcs/
    external_repositories.md
```

## Active Commands

Spark-side SLAM workspace in this repository:

```bash
cd /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam
```

Current best realtime profile:

```bash
scripts/run_slam_live_demo_1440.sh
```

The original live working directory remains available on Spark:

```text
/home/deep/peize/where_is_my_key/x4_slam
```

## Local Assets

The repository intentionally does not store generated data or large binary assets. A fresh checkout needs local SDK and vocabulary assets before full live SLAM can be rebuilt:

- Insta360 ARM64 CameraSDK headers and `libCameraSDK.so`
- ORB vocabulary at `x4_slam/config/orb_vocab.fbow`
- Third-party Stella VSLAM dependencies under `x4_slam/third_party/install/`

On the Spark machine, recreate the local symlinks with:

```bash
cd /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam
scripts/link_local_assets.sh
```

See [docs/open_source_compliance.md](docs/open_source_compliance.md) before
adding third-party code or binary assets.

## Important Notes

- The Spark robot computer is ARM64, not x86.
- The current camera is Insta360 X4. X5 was only an early stand-in.
- Do not commit bags, maps, build products, SDK binaries, ORB vocab files, or large generated artifacts.
- GitHub CLI auth is currently invalid on this Spark machine, so local commits can be prepared but pushing requires re-authentication.
