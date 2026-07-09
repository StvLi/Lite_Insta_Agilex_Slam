# Insta360 X4 360 VSLAM 实现方案

日期：2026-07-06  
工作区：`/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam`

## 0. 当前部署状态

截至 2026-07-09，基础相机链路已在当前 ARM64 主机上跑通，当前相机已从早期代用的 X5 换成 Insta360 X4：

- 当前目标相机是 Insta360 X4；早期 X5 曾在 Android 模式下被官方 CameraSDK 2.1.1 发现并打开。
- 已安装 udev 规则 `/etc/udev/rules.d/99-insta360.rules`，ROS 节点无需 root 即可访问 `2e1a` USB 设备。
- 已优先采用社区 `Longxiaoze/insta360_ros_driver` 路线，并在本工作区完成 Jazzy + ARM64 + SDK 2.1.1 适配。
- 默认 bringup 可以发布压缩双鱼眼、解码双鱼眼、equirectangular 图像、原始 IMU、Madgwick IMU 和 TF。
- 已将 `stella_vslam_ros` 接入 `ros2_ws`，并在本机 ARM64 下构建本地 `stella_vslam`、`FBoW`、`g2o` 依赖。
- 已下载 ORB 词袋到 `config/orb_vocab.fbow`，并建立 `config/insta360X4_equirectangular.yaml` 作为当前 `/camera/image_raw` 的 equirectangular 配置。
- 当前 `data/bags/smoke_test_002` 可以验证图像 transport 和 SLAM 启动，但没有成功初始化 monocular map；下一步需要录制具备平移视差和丰富纹理的验收 bag。
- 详细状态记录见 `docs/deployment_status.md`。

当前验证数据：

```text
/dual_fisheye/image/compressed  about 29.5 Hz
/dual_fisheye/image             about 15.9 Hz, 3840x1920, bgr8
/camera/image_raw               about 12.8 Hz, 3840x1920, bgr8
/imu/data_raw                   about 52 Hz
/imu/data                       about 52 Hz
```

当前路线更新：先使用已跑通的社区驱动作为相机接入和图像展开基线；若后续社区驱动在 X4、标定、性能或 SLAM 接口上遇到硬阻断，再拆出自研 `camera_sdk_bridge`。

## 1. 背景与目标

本项目目标是在 NVIDIA DGX Spark 上构建一套基于 Insta360 X4 的 360 度视觉 SLAM 系统。早期曾使用 Insta360 X5 代用，现在当前相机已切换为 X4，因此实现时必须避免把设备逻辑硬绑定到 X5 或 X4 的单一型号细节。

目标输出包括：

- 稳定接入 Insta360 X4/X5 相机。
- 获取实时 360/双鱼眼视频流与 IMU 数据。
- 将相机数据发布为 ROS2 topic。
- 通过 stella_vslam / stella_vslam_ros 跑通实时定位建图。
- 形成可替换相机、可复现实验、可逐步优化的工程结构。

## 2. 当前设备与约束

主机：

- 型号：NVIDIA DGX Spark
- 架构：ARM64 / AArch64
- 系统：Ubuntu 24.04.3 LTS
- 内存：128GB CPU/GPU 共享内存

相机：

- 当前设备：Insta360 X4
- 早期临时代用：Insta360 X5
- 早期 X5 曾被系统识别为 UVC 摄像头：`/dev/video0`、`/dev/video1`
- 官方 SDK 控制路线要求相机连接后在屏幕上选择 `Android` 模式；普通 UVC 摄像头模式下 SDK demo 会显示 `no device found`

重要约束：

- 所有二进制依赖必须优先确认 ARM64 兼容性。
- 不使用 x86_64 / amd64 的 SDK 库或 deb 包。
- Insta360 MediaSDK 当前官网包只提供 amd64，不适合本机主路线。
- 后续 ROS/SLAM 依赖要按 Ubuntu 24.04 + ARM64 重新确认，不照搬 amd64/Ubuntu 22.04 文档。

## 3. 参考资料位置

参考仓库：

- 官方 SDK 示例仓库：`/home/deep/peize/where_is_my_key/ref_repo/src/Desktop-CameraSDK-Cpp`
- 社区 X4 VSLAM 项目：`/home/deep/peize/where_is_my_key/ref_repo/src/360Vslam`

官网申请 SDK：

- 原始 zip：`/home/deep/peize/where_is_my_key/ref_repo/src/8c916a0759ab166ece033b66affb8118_6831459114181497998_m.zip`
- 解压目录：`/home/deep/peize/where_is_my_key/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1`

推荐使用的 CameraSDK：

```text
/home/deep/peize/where_is_my_key/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1/CameraSDK-20251105_140609-2.1.1-gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu
```

备选 CameraSDK：

```text
CameraSDK-20251105_145908-2.1.1-gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu
CameraSDK-20251105_112855-2.1.1-jetson-linux-9.3.0-2020.08-x86_64_aarch64_linux-gnu
```

不推荐在本机使用：

```text
CameraSDK-20251104_115504-2.1.1.1-Linux                 # x86-64
libMediaSDK-dev-3.1.1.0-amd64.tar_1758540334111.xz      # amd64 only
```

## 4. 推荐总体架构

建议分成四层：

1. 相机接入层 `camera_sdk_bridge`
   - 使用 Insta360 CameraSDK。
   - 负责发现设备、打开设备、启动 live stream、接收视频/音频/IMU 回调。
   - 兼容 X4/X5，启动时打印 `camera_name`、`camera_type`、`fw_version`。

2. 解码与图像转换层 `stream_decoder`
   - SDK 回调给出 H.264/H.265 码流。
   - 通过 FFmpeg/libavcodec 解码成 OpenCV `cv::Mat`。
   - 对双鱼眼图像进行必要旋转、拼接或转换。
   - 初期优先输出 dual-fisheye，后续再做 equirectangular。

3. ROS2 发布层 `ros_camera_node`
   - 发布图像：`/insta360/dual_fisheye/image_raw`
   - 发布 IMU：`/insta360/imu/data_raw`
   - 发布相机信息和状态：`/insta360/status`
   - 后续可增加 `/insta360/equirectangular/image_raw`

4. SLAM 层 `slam_pipeline`
   - 使用 stella_vslam_ros 读取 360/equirectangular 图像。
   - 加载 ORB vocabulary。
   - 使用 X4/X5 对应相机模型和标定 yaml。
   - 输出 map、trajectory、定位状态。

## 5. 工作区建议结构

```text
x4_slam/
  IMPLEMENTATION_PLAN.md
  third_party/
    README.md
  sdk/
    README.md
    CameraSDK -> <指向推荐 ARM64 SDK 的软链接>
  ros2_ws/
    src/
      insta360_sdk_bridge/
      insta360_image_tools/
      x4_slam_bringup/
      x4_slam_config/
  scripts/
    check_camera_usb.sh
    run_sdk_demo.sh
    build_ros_ws.sh
    run_camera_driver.sh
    run_bringup.sh
    run_slam.sh
  docs/
    calibration.md
    deployment_status.md
    troubleshooting.md
  data/
    bags/
    maps/
    calibration/
```

不要把大体积 SDK 二进制复制多份进工作区。建议在 `x4_slam/sdk/CameraSDK` 放一个软链接指向 ref_repo 下的官方 ARM64 SDK。

当前实际 ROS2 package 位于：

```text
x4_slam/ros2_ws/src/insta360_ros_driver
```

构建和运行脚本：

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/build_ros_ws.sh
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_camera_driver.sh
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_bringup.sh
```

## 6. 第一阶段：相机与 SDK 验证

目标：确认 SDK 可以发现并打开 X4。早期 X5 验证结果只作为链路参考。

检查 USB：

```bash
lsusb | grep -i 'insta\|2e1a'
ls -l /dev/bus/usb/*/*
ls -l /dev/video* /dev/v4l/by-id/*Insta* 2>/dev/null
```

切换相机模式：

- X4/X5 连接 USB 后，在相机屏幕选择 `Android` 模式。
- 如果系统只出现 `/dev/video0`，通常说明当前走的是 UVC 摄像头模式，不一定能被 CameraSDK 发现。

运行 SDK demo：

```bash
export SDK=/home/deep/peize/where_is_my_key/ref_repo/src/Linux_CameraSDK-2.1.1_MediaSDK-3.1.1/CameraSDK-20251105_140609-2.1.1-gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu
export LD_LIBRARY_PATH="$SDK/lib:$LD_LIBRARY_PATH"
"$SDK/bin/CameraSDKTest"
```

如果仍显示 `no device found`：

- 确认相机是否在 Android 模式。
- 检查是否有 udev 权限问题。
- 临时测试可用 root 跑 demo；长期方案应增加 udev 规则，而不是依赖 sudo。
- 记录 `lsusb -v -d 2e1a:` 输出，比较 Android 模式和 UVC 模式的 USB interface 差异。

## 7. 第二阶段：最小 C++ SDK 程序

目标：做一个非 ROS 的最小程序，验证 SDK API 和视频回调。

核心流程：

```cpp
ins_camera::DeviceDiscovery discovery;
auto devices = discovery.GetAvailableDevices();
auto camera = std::make_shared<ins_camera::Camera>(devices[0].info);
camera->Open();
camera->SetStreamDelegate(delegate);

if (camera_type >= ins_camera::CameraType::Insta360X4) {
    camera->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_LIVEVIEW);

    ins_camera::RecordParams record_params;
    record_params.resolution = ins_camera::VideoResolution::RES_3840_1920P30;
    record_params.bitrate = 0;
    camera->SetVideoCaptureParams(
        record_params,
        ins_camera::CameraFunctionMode::FUNCTION_MODE_LIVE_STREAM
    );
}

ins_camera::LiveStreamParam param;
param.video_resolution = ins_camera::VideoResolution::RES_3840_1920P30;
param.lrv_video_resulution = ins_camera::VideoResolution::RES_1440_720P30;
param.video_bitrate = 1024 * 1024 * 10;
param.enable_audio = false;
param.enable_gyro = true;
param.using_lrv = false;
camera->StartLiveStreaming(param);
```

X5 支持的预览流分辨率在 SDK example 中标注为：

- `RES_3840_1920P30`
- `RES_2560_1280P30`
- `RES_2160_1080P30`
- `RES_1920_960P30`

初期建议使用 `RES_1920_960P30` 或 `RES_3840_1920P30`：

- `1920x960` 更利于快速验证和低延迟。
- `3840x1920` 更接近 SLAM 可用质量，但解码和特征提取压力更大。

## 8. 第三阶段：ROS2 相机桥接

目标：创建 ROS2 package，把 SDK 数据变成标准 topic。

建议 package：

```text
insta360_sdk_bridge
```

节点：

```text
insta360_camera_node
```

参数：

```yaml
camera:
  prefer_model: "X4"
  allow_x5_fallback: true
  sdk_path: "/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/sdk/CameraSDK"
  resolution: "RES_1920_960P30"
  bitrate: 10000000
  enable_audio: false
  enable_gyro: true
  using_lrv: false

topics:
  dual_fisheye: "/insta360/dual_fisheye/image_raw"
  equirectangular: "/insta360/equirectangular/image_raw"
  imu: "/insta360/imu/data_raw"
```

发布 topic：

```text
/insta360/dual_fisheye/image_raw          sensor_msgs/msg/Image
/insta360/imu/data_raw                    sensor_msgs/msg/Imu
/insta360/status                          diagnostic_msgs/msg/DiagnosticArray 或自定义状态
```

注意：

- SDK 视频回调线程中不要直接做重计算，先进入队列。
- 解码线程和 ROS 发布线程分离。
- 图像 timestamp 优先使用 SDK 回调中的 `timestamp`，不要只用 `now()`。
- IMU timestamp 要和图像时间系统一致；初期可记录偏移，后续再做同步修正。

## 9. 第四阶段：图像处理与标定

目标：为 SLAM 提供稳定的 equirectangular 图像和相机配置。

阶段策略：

1. 先发布原始 dual-fisheye 图像，确认码流、帧率、IMU 都稳定。
2. 复用社区 `insta360_ros_driver` 的思路，做 fisheye 到 equirectangular 的转换。
3. 为 X4 生成独立标定文件，X5 临时代用时保留 X5 标定。
4. stella_vslam 使用 equirectangular 相机模型。

标定产物建议：

```text
config/calibration/x4_dual_fisheye.yaml
config/calibration/x4_equirectangular.yaml
config/calibration/x5_dual_fisheye.yaml
config/calibration/x5_equirectangular.yaml
```

不要把早期 X5 标定当成 X4 的最终标定。当前应以 X4 重新确认标定与配置。

## 10. 第五阶段：stella_vslam 集成

目标：从实时 ROS2 图像 topic 跑通 SLAM。

依赖方向：

- `stella_vslam`
- `stella_vslam_ros`
- `FBoW_orb_vocab`
- OpenCV
- g2o
- yaml-cpp
- sqlite3

社区项目文档按 ROS Humble / Ubuntu 22.04 写，本机是 Ubuntu 24.04。建议优先确认 ROS2 Jazzy 是否可用；如果必须复用 Humble 生态，则考虑容器或独立 Ubuntu 22.04 环境，但容器也必须使用 ARM64 镜像。

当前接入状态：

- `stella_vslam_ros` 位于 `ros2_ws/src/stella_vslam_ros`。
- `stella_vslam`、`FBoW`、`g2o` 安装在 `third_party/install`。
- `run_slam.sh` 会导出本地库路径并加载默认词袋/配置。
- 默认订阅 `/camera/image_raw`，输出 `/run_slam/camera_pose`。

运行形态：

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

如需切换输入图像 topic：

```bash
IMAGE_TOPIC=/your/equirectangular/topic \
  /home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

离线 bag 验证：

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/play_smoke_bag.sh
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_slam_outputs.sh
```

当前 `smoke_test_002` 只算 transport smoke test。最终验收需要 `/run_slam/camera_pose` 有持续输出、map 非空、trajectory 文件有轨迹行。详细步骤见 `docs/slam_validation.md`。

## 11. 解码策略

SDK `OnVideoData` 输出 H.264 或 H.265 码流，具体编码可通过：

```cpp
camera->GetVideoEncodeType();
```

建议实现顺序：

1. 软件解码：FFmpeg/libavcodec，先跑通。
2. 评估硬件解码：本机 NVIDIA/ARM 环境中确认可用 decoder 后再接入。
3. 如果使用 Jetson 相关参考，不直接照搬 `h264_cuvid`；需要实际检查本机 `ffmpeg -decoders` 输出。

当前机器上 `ffmpeg` 尚未确认安装。安装和构建阶段要注意 ARM64 包源。

## 12. 权限与 udev 策略

长期不建议依赖 `sudo ./CameraSDKTest`。

建议添加 udev 规则：

```text
SUBSYSTEM=="usb", ATTR{idVendor}=="2e1a", MODE="0666", TAG+="uaccess"
```

规则文件建议：

```text
/etc/udev/rules.d/99-insta360.rules
```

应用后：

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

如果无法改系统规则，开发脚本中至少要检测权限并给出明确错误信息。

## 13. 里程碑

M0：环境确认

- 确认 ROS2 版本选择。
- 确认 FFmpeg/libavcodec 可用。
- 确认 CameraSDK ARM64 demo 可运行。

M1：SDK 发现相机

- X5 Android 模式下被 SDK 发现。
- `CameraSDKTest` 可打开相机并进入菜单。
- 记录 X5 和未来 X4 的 `camera_name`、`camera_type`、`fw_version`。

M2：视频流保存

- 使用 SDK live stream 保存 `.h264` 或 `.h265`。
- 能用 FFmpeg 解码出图像。
- 确认不同分辨率帧率表现。

M3：ROS2 bridge

- 发布 `/insta360/dual_fisheye/image_raw`。
- 发布 `/insta360/imu/data_raw`。
- 可用 rosbag 录制。

M4：equirectangular 转换

- 输出 `/insta360/equirectangular/image_raw`。
- 建立 X5 临时标定。
- X4 到手后建立 X4 标定。

M5：离线 SLAM

- 使用录制 bag 跑 stella_vslam。
- 输出 map 和轨迹。
- 调整相机 yaml 和 ORB 参数。

M6：实时 SLAM

- 相机实时流接入 stella_vslam_ros。
- 监控 CPU/GPU/内存/帧率/延迟。
- 固化 launch 文件和运行脚本。

## 14. 风险清单

- SDK 模式风险：UVC 模式能被 Linux 当 webcam 使用，但 SDK 可能无法发现。
- 架构风险：社区文档中有 amd64 deb，不能在本机使用。
- MediaSDK 风险：官网包只提供 amd64，不适合 ARM64 主机。
- ROS 版本风险：社区项目基于 ROS Humble，本机 Ubuntu 24.04 更自然对应 ROS Jazzy。
- 标定风险：X5 和 X4 参数不同，X5 只能用于链路验证。
- 解码风险：H.265/H.264 硬解接口与 Jetson 文档可能不同，需要实测。
- 时间同步风险：SLAM 对 timestamp 敏感，不能长期用简单 `now()` 替代相机时间。
- 性能风险：高分辨率 360 图像特征提取压力大，需要在分辨率、帧率、画质之间取平衡。

## 15. 推荐下一步

1. 录制 `slam_acceptance_001`：45-90 秒、纹理丰富、包含明显平移视差，避免纯原地旋转。
2. 用 `run_slam.sh` + `play_smoke_bag.sh` + `check_slam_outputs.sh` 离线验收 `/run_slam/camera_pose`。
3. 若仍无法初始化，优先调整运动采集方式，再调 equirectangular mask、ORB 阈值、分辨率/帧率。
4. 离线 bag 跑通后，再切回 live camera 做实时验收。
5. X4 到手后重复 SDK demo、ROS bringup、topic size/rate、SLAM 初始化与标定检查，替换配置而不是重写软件链路。
