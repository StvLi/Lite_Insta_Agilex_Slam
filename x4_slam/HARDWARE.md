# Hardware

Date: 2026-07-09

All active ROS machines are configured with:

```text
ROS_DOMAIN_ID=15
```

## Main Development Workstation

```text
Host: stvli@192.168.88.12
Role: main development and visualization workstation
Architecture: x86_64
OS: Ubuntu 24.04
ROS: ROS2 Jazzy
Workspace: /home/stvli/Desktop/where_is_my_key
Display: yes
```

This is where RViz and operator-facing visualization should run.

## Robot Local Computer

```text
Host: deep@192.168.88.11
Role: robot local compute computer
Machine: NVIDIA DGX Spark
Architecture: ARM64 / AArch64
OS: Ubuntu 24.04.3 LTS
ROS: ROS2 Jazzy
Workspace: /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam
Display: no
Network checked: 192.168.88.11/24 on enP7s7
```

Important traits:

- ARM device, not x86.
- 128GB unified/shared memory.
- Runs the Insta360 camera and SLAM stack headlessly.
- Current camera is Insta360 X4.

## Camera

```text
Current camera: Insta360 X4
Previous development stand-in: Insta360 X5
Expected SDK mode: Android / vendor-specific USB mode
```

Verification note on 2026-07-09: Spark was reachable and ROS_DOMAIN_ID was set, but no Insta360 USB device was enumerated at check time. Recheck with:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_camera_usb.sh
```

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

This board is connected but not involved in the current SLAM debugging path yet.

## Credential Handling

Passwords were provided by the operator in conversation for authorized operations. They are intentionally not stored in project files.
