# Hardware Topology

Date: 2026-07-09

All active ROS machines are configured with:

```text
ROS_DOMAIN_ID=15
```

## Main Development Workstation

```text
Host: stvli@192.168.88.12
Role: primary development and visualization workstation
Architecture: x86_64
OS: Ubuntu 24.04
ROS: ROS2 Jazzy
Assigned workspace: /home/stvli/Desktop/where_is_my_key
Display: yes
```

Use this machine as the main desktop/dev environment and the target RViz visualization host.

## Robot Local Computer

```text
Host: deep@192.168.88.11
Role: robot local compute computer
Machine: NVIDIA DGX Spark
Architecture: ARM64 / AArch64
OS: Ubuntu 24.04
ROS: ROS2 Jazzy
Workspace: /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam
Display: no
```

This machine currently runs the Insta360 camera/SLAM stack. It is typically accessed from the main development workstation over SSH.

Remembered hardware traits:

- ARM architecture, not x86.
- 128GB unified/shared memory.
- Current camera is Insta360 X4.
- Previous development stand-in was Insta360 X5.

## Motion-Domain Controller

```text
Host: sunrise@192.168.88.10
Role: robot motion-domain controller
Board: D-Robotics / Horizon X5 development board
Architecture: ARM
OS: Ubuntu 22.04
ROS: ROS2 Humble with vendor optimizations
Display: no
```

This board is connected but not part of the current SLAM debugging path yet.

## Credential Handling

Passwords were provided by the operator in conversation for authorized operations, but they are intentionally not stored in project files.
