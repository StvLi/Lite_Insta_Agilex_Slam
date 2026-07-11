# External Repositories

Date: 2026-07-09

## Active

```text
Main repository:
https://github.com/StvLi/Lite_Insta_Agilex_Slam
```

The active `x4_slam` code is maintained directly inside the main repository.

## Upstream References

```text
Original X4 SLAM reference:
https://github.com/Longxiaoze/360Vslam

Active driver base used by our current Spark implementation:
https://github.com/Longxiaoze/insta360_ros_driver
```

`Longxiaoze/360Vslam` remains useful as a historical/reference upstream. The
live Spark pipeline uses an adapted `Longxiaoze/insta360_ros_driver` copy plus
`stella_vslam_ros`; it does not run the original `360Vslam/main.cpp` directly.

`StvLi/360Vslam` is not used as a project subrepository. It was considered as
a possible split, but the project now keeps the SLAM code in the main
repository for simpler maintenance.

## Known Future Dependencies

```text
Lite robot ROS2 control:
https://github.com/TeamLite-DeepCybo/lite_ros2

Agilex chassis / NAVIS API integration:
/home/stvli/Desktop/where_is_my_key/src/Lite_Agilex_API
```

The NAVIS secondary-development API wrapper lives in `Lite_Agilex_API`.
