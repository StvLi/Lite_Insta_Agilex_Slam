# Hardware Topology

Date: 2026-07-09

All active ROS machines use:

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

## Robot Local Computer

```text
Host: deep@192.168.88.11
Role: robot local compute computer
Machine: NVIDIA DGX Spark
Architecture: ARM64 / AArch64
OS: Ubuntu 24.04.3 LTS
ROS: ROS2 Jazzy
Display: no
```

Important traits:

- ARM architecture, not x86.
- 128GB unified/shared memory.
- Runs Insta360 X4 camera and SLAM headlessly.

## Camera

```text
Current camera: Insta360 X4
Previous stand-in: Insta360 X5
Expected mode: Android / CameraSDK USB mode
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

This board is connected but not in the current SLAM debugging path.

## Credential Handling

Passwords were provided by the operator for authorized operations. They must not be committed to project files.
