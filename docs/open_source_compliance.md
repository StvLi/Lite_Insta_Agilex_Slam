# Open Source And Binary Asset Compliance

Date: 2026-07-09

## Policy

Keep source code, local patches, configuration, and upstream license files in
Git. Keep proprietary SDK files, large binary vocabularies, generated maps,
bags, build products, and logs out of Git.

## Included Source

| Component | Source | License files kept in tree | Notes |
| --- | --- | --- | --- |
| `insta360_ros_driver` | Adapted from `Longxiaoze/insta360_ros_driver` | `x4_slam/ros2_ws/src/insta360_ros_driver/LICENSE.txt` | Apache-2.0 source and local ROS2 Jazzy/ARM64/resolution changes are kept in the main repo. |
| `stella_vslam_ros` | Upstream `stella_vslam_ros` source snapshot | `LICENSE`, `LICENSE.fork`, `LICENSE.original` | BSD-2-Clause notices are retained. |
| `stella_vslam_ros/3rd/filesystem` | Upstream third-party source bundled by `stella_vslam_ros` | `3rd/filesystem/LICENSE` | MIT license retained. |
| `stella_vslam_ros/3rd/json` | Upstream third-party source bundled by `stella_vslam_ros` | `3rd/json/LICENSE` | MIT license retained. |
| `stella_vslam_ros/3rd/popl` | Upstream third-party source bundled by `stella_vslam_ros` | `3rd/popl/LICENSE` | MIT license retained. |
| `stella_vslam_ros/3rd/spdlog` | Upstream third-party source bundled by `stella_vslam_ros` | `3rd/spdlog/LICENSE` | MIT license retained. |

## Reference-Only Upstreams

| Repository | Use |
| --- | --- |
| `Longxiaoze/360Vslam` | Reference route only. We did not import or run its `main.cpp` as the active runtime. |

## Local Binary Assets Not Committed

| Asset | Local path | Reason |
| --- | --- | --- |
| Insta360 CameraSDK headers and `libCameraSDK.so` | `x4_slam/sdk/CameraSDK`, `x4_slam/ros2_ws/src/insta360_ros_driver/include/camera`, `include/stream`, `lib/libCameraSDK.so` | Official SDK files are treated as vendor/proprietary assets. |
| ORB vocabulary | `x4_slam/config/orb_vocab.fbow` | Large external binary asset. |
| Stella VSLAM binary dependencies | `x4_slam/third_party/install` | Generated/local build products. |
| ROS bags, maps, logs, eval output | `x4_slam/data/*` | Generated experiment output. |

Run `x4_slam/scripts/link_local_assets.sh` after checkout to recreate the local
symlinks expected by the build scripts.

Before pushing, check:

```bash
git status --short --ignored
git add --dry-run .
git ls-files -o --exclude-standard | grep -E '\.(so|fbow|mcap|db3|zip|tar\.gz)$' || true
```
