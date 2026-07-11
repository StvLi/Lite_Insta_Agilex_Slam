<div align="center">

<img src="https://img.shields.io/badge/ROS2-Jazzy-22314E?logo=ros" alt="ROS2 Jazzy">
<img src="https://img.shields.io/badge/Ubuntu-24.04-E95420?logo=ubuntu" alt="Ubuntu 24.04">
<img src="https://img.shields.io/badge/Platform-ARM64%20%7C%20x86__64-lightgrey" alt="ARM64 | x86_64">
<img src="https://img.shields.io/badge/License-MIT-blue" alt="MIT License">

<h1>🤖 Lite Insta Agilex Slam</h1>
<h3>DeepCybo Lite — 具身智能机器人全栈系统</h3>
<h3>Embodied AI Robot Full-Stack System</h3>

</div>

---

## 📋 项目简介 | Overview

> **English** · This is the top-level orchestrator repository for the DeepCybo Lite dual-arm humanoid robot. It integrates perception (SLAM), chassis mobility, motion planning, and embodied agent control into a unified system — all driven by ROS 2 Jazzy.

> **中文** · DeepCybo Lite 双臂人形机器人顶层编排仓库。整合感知建图、底盘移动、运动规划与具身智能体控制，统一运行于 ROS 2 Jazzy。

**核心硬件 Core Hardware**:
- 🤖 Lite dual-arm humanoid robot · Lite 双臂人形机器人
- 📷 Insta360 X4 panoramic camera · Insta360 X4 全景相机
- 🚜 Agilex RANGER mobile chassis · 松灵 RANGER 移动底盘
- 🖥️ NVIDIA DGX Spark (ARM64) onboard computer · 机载计算机

---

## 🏗️ 仓库架构 | Repository Architecture

```text
Lite_Insta_Agilex_Slam/          ← 🎯 主仓库 Main Repo (you are here)
├── x4_slam/                    ← 📷 视觉 SLAM 子系统 Visual SLAM Subsystem
│   ├── ros2_ws/src/
│   │   ├── insta360_ros_driver/  # Insta360 相机驱动
│   │   └── stella_vslam_ros/     # Stella VSLAM ROS2 封装
│   ├── config/                   # 相机 & SLAM 配置
│   ├── scripts/                  # 建图/定位/可视化脚本
│   └── sdk/                      # Insta360 CameraSDK
│
├── Lite_Agilex_API/            ← 🚜 底盘 API 子系统 Chassis API Subsystem
│   ├── python/agilex_client/     # Python HTTP/WS 客户端
│   ├── ros2_ws/src/
│   │   └── agilex_chassis_bridge/# ROS2 底盘桥接节点
│   ├── scripts/                  # 建图/导航/位姿脚本
│   └── web/                      # Web 交互地图
│
├── lite_moveit2/               ← 🦾 运动规划子系统 Motion Planning Subsystem
│   ├── config/                   # MoveIt2 SRDF & kinematics
│   ├── launch/                   # MoveIt2 启动文件
│   ├── scripts/                  # 手臂位姿控制脚本
│   └── urdf/                     # Lite 机器人 URDF 模型
│
├── docs/                       ← 📖 文档 Documentation
├── vcs/                        ← 🔗 外部依赖清单 External Dependencies
└── (agent/)                    ← 🧠 具身智能体 · 即将到来 Coming Soon
```

---

## 🚀 快速开始 | Quick Start

### 克隆仓库 | Clone

```bash
git clone --recurse-submodules https://github.com/StvLi/Lite_Insta_Agilex_Slam.git
cd Lite_Insta_Agilex_Slam
```

> ⚠️ 如果已克隆但未拉取子模块 | If cloned without submodules:
> ```bash
> git submodule update --init --recursive
> ```

### 子系统 | Subsystems

| 子系统 Subsystem | 功能 Function | 入口 Entry Point |
| :--- | :--- | :--- |
| **x4_slam** | Insta360 X4 视觉 SLAM | [`x4_slam/README.md`](x4_slam/README.md) |
| **Lite_Agilex_API** | 松灵底盘建图/导航 | [`Lite_Agilex_API/README.md`](Lite_Agilex_API/README.md) |
| **lite_moveit2** | Lite 双臂运动规划 | [`lite_moveit2/package.xml`](lite_moveit2/package.xml) |
| **agent** *(coming)* | 具身智能体调度 | — |

---

## 🔧 子仓库说明 | Submodule Descriptions

### 📷 x4_slam — 视觉 SLAM

> Insta360 X4 panoramic visual SLAM pipeline running on the NVIDIA DGX Spark (ARM64). Supports live camera feed and rosbag-based off-line SLAM with remote RViz visualization.

> 基于 Insta360 X4 的全景视觉 SLAM，运行于 NVIDIA DGX Spark (ARM64)。支持实时相机流和 rosbag 离线建图，配合远程 RViz 可视化。

- **Sensors**: Insta360 X4 @ 1440p equirectangular
- **SLAM Backend**: Stella VSLAM (ORB-feature based)
- **Network**: ROS 2 Jazzy, `ROS_DOMAIN_ID=15`

### 🚜 Lite_Agilex_API — 底盘控制

> HTTP/WebSocket API client for the Agilex RANGER chassis. Provides mapping, navigation, pose query, and laser-map visualization. Designed to support embodied agent function-calling.

> 松灵 RANGER 底盘 HTTP/WebSocket API 客户端。提供建图、导航、位姿查询、激光地图可视化，为具身 Agent function-calling 设计。

- **Interface**: HTTP REST + WebSocket
- **Bridge**: ROS 2 `agilex_chassis_bridge` node
- **Agent API**: JSON-based `agent_get_map` / `agent_get_pose` / `agent_set_target_pose`

### 🦾 lite_moveit2 — 运动规划

> MoveIt 2 configuration and motion-planning scripts for the DeepCybo Lite dual-arm robot. Supports Cartesian translation, joint-space planning, and time-optimal trajectories.

> DeepCybo Lite 双臂机器人 MoveIt 2 配置与运动规划脚本。支持笛卡尔平移、关节空间规划和时间最优轨迹。

- **Planner**: Pilz Industrial Motion Planner + RRTConnect
- **Kinematics**: KDL / TRAC-IK
- **Control**: ros2_control + joint_trajectory_controller

---

## 🧠 具身智能体（即将推出）| Embodied Agent (Coming Soon)

The agent subsystem will orchestrate all three subsystems through function-calling:
智能体子系统将通过 function-calling 统一调度三个子系统：

```text
User Prompt → Agent Core
                 ├── 🗺️  SLAM: "where am I?" → x4_slam
                 ├── 🚜  Nav:  "go to (x,y)" → Lite_Agilex_API
                 └── 🦾  Arm:  "pick object" → lite_moveit2
```

---

## 📁 目录约定 | Directory Conventions

- **不提交/Do not commit**: rosbags, maps, build products, SDK binaries, ORB vocab files, or large generated artifacts.
- **build/ install/ log/**: Colcon 构建产物，已在 `.gitignore` 中排除。
- **docs/**: 硬件拓扑、部署方案、开源合规等工程文档。

---

## 🌐 网络拓扑 | Network Topology

| 节点 Node | IP | 架构 Arch | 系统 OS | 角色 Role |
| :--- | :--- | :--- | :--- | :--- |
| Spark (deep) | 192.168.88.11 | ARM64 | Ubuntu 24.04 | SLAM 机载计算机 |
| Workstation (stvli) | 192.168.88.12 | x86_64 | Ubuntu 24.04 | 开发/可视化 |
| Motion Ctrl (sunrise) | 192.168.88.10 | ARM | Ubuntu 22.04 | 运动控制器 |

`ROS_DOMAIN_ID=15` on all machines.

See [`docs/hardware_topology.md`](docs/hardware_topology.md) for details.

---

## 🌟 致谢 | Acknowledgments

- [Insta360 CameraSDK](https://github.com/Archer11H/insta360_ros_driver) — 360° camera driver
- [Stella VSLAM](https://github.com/stella-cv/stella_vslam) — Visual SLAM framework
- [MoveIt 2](https://moveit.ai/) — Motion planning framework
- [Agilex Robotics](https://www.agilex.ai/) — Chassis platform

---

<div align="center">

**DeepCybo · Team Lite**

<sub>Built with ❤️ for embodied AI · 为具身智能而生</sub>

</div>
