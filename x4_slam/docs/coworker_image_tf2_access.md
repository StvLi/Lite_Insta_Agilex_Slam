# Stamped Image And TF2 Access For DDGS

This note is for the co-worker agent that needs on-demand Insta360 X4 images and
time-aligned SLAM camera poses.

## Runtime Assumptions

- Robot computer: Spark, `deep@192.168.88.11`
- ROS distro: Jazzy
- ROS domain: `ROS_DOMAIN_ID=15`
- SLAM image topic on Spark: `/camera/image_raw`
- On-demand image service: `/camera/get_stamped_image`
- Source-stamped camera TF for reconstruction: `map -> camera_frame`
- Visualization TF: `map -> slam_camera`

Do not subscribe to `/camera/image_raw` from the development workstation unless
you explicitly want to consume bridge bandwidth. Use the service below for
single-frame pulls.

## Start The Provider

On Spark, start the normal headless SLAM pipeline:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_headless_1440.sh
```

This starts:

- `equirectangular_node`, which provides `/camera/get_stamped_image`
- `stella_vslam_ros`, which publishes `/run_slam/camera_pose`
- `odom_to_tf.py`, which publishes:
  - `map -> slam_camera` for RViz/live display
  - `map -> camera_frame` with the original SLAM pose timestamp for reconstruction

If launching manually, make sure the TF bridge is started with:

```bash
TF_SOURCE_CHILD_FRAME=camera_frame /path/to/x4_slam/scripts/odom_to_tf.py
```

## Service Contract

Service type:

```text
insta360_ros_driver/srv/GetStampedImage
```

Request:

```text
uint32 width
uint32 height
string encoding
```

Response:

```text
bool success
string message
builtin_interfaces/Time stamp
string frame_id
sensor_msgs/Image image
```

Request defaults:

- `width=0` and `height=0`: return the current SLAM stream resolution. In the
  standard headless profile this is `1440x720`.
- one of `width` or `height` set to `0`: preserve aspect ratio.
- `encoding=""`, `passthrough`, or `source`: return source encoding.

Supported output encodings:

- `bgr8`
- `rgb8`
- `mono8`

The returned `response.stamp` is identical to `response.image.header.stamp`.
The returned `response.frame_id` is identical to `response.image.header.frame_id`.
Under the normal pipeline this frame is `camera_frame`.

## Quick Smoke Test

On a client machine with the updated workspace built and sourced:

```bash
export ROS_DOMAIN_ID=15
export ROS_AUTOMATIC_DISCOVERY_RANGE=SUBNET
source /opt/ros/jazzy/setup.bash
source /path/to/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/install/setup.bash

ros2 service type /camera/get_stamped_image
ros2 service call /camera/get_stamped_image insta360_ros_driver/srv/GetStampedImage \
  "{width: 1440, height: 720, encoding: 'bgr8'}"
```

For a less noisy file-writing test:

```bash
/path/to/Lite_Insta_Agilex_Slam/x4_slam/scripts/request_stamped_image.py \
  --width 1440 \
  --height 720 \
  --encoding bgr8 \
  --output /tmp/insta360_frame.png
```

## X86 Smoke Test

Spark has no display, so run the visual smoke test from the x86 development
workstation. The test does not open a GUI. It requests one frame, saves it to a
file, then queries the source-stamped TF at the exact image timestamp.

On Spark:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam_headless_1440.sh
```

On the x86 workstation:

```bash
conda activate lite_ros2_env
export ROS_DOMAIN_ID=15
export ROS_AUTOMATIC_DISCOVERY_RANGE=SUBNET
source /opt/ros/jazzy/setup.bash
source /home/stvli/Desktop/where_is_my_key/src/Lite_Insta_Agilex_Slam/x4_slam/ros2_ws/install/setup.bash

/home/stvli/Desktop/where_is_my_key/src/Lite_Insta_Agilex_Slam/x4_slam/scripts/ddgs_image_tf2_smoke_test.py \
  --width 1440 \
  --height 720 \
  --encoding bgr8 \
  --output /tmp/ddgs_stamped_image.png
```

Expected output includes:

```text
image_request: ok
image_stamp: <sec>.<nanosec>
image_frame_id: camera_frame
image_size: 1440x720
image_file: /tmp/ddgs_stamped_image.png
tf_frames: map <- camera_frame
T_target_source:
  ...
```

Open `/tmp/ddgs_stamped_image.png` on the x86 machine to inspect the image. If
the image is saved but TF lookup fails, SLAM has not yet produced a pose for
that image timestamp; wait for tracking to initialize and retry.

## Python Client Pattern

```python
import rclpy
from cv_bridge import CvBridge
from insta360_ros_driver.srv import GetStampedImage

rclpy.init()
node = rclpy.create_node("ddgs_image_client")
client = node.create_client(GetStampedImage, "/camera/get_stamped_image")
client.wait_for_service(timeout_sec=5.0)

req = GetStampedImage.Request()
req.width = 1440
req.height = 720
req.encoding = "bgr8"

future = client.call_async(req)
rclpy.spin_until_future_complete(node, future, timeout_sec=5.0)
resp = future.result()
if not resp or not resp.success:
    raise RuntimeError(resp.message if resp else "image service timeout")

stamp_msg = resp.image.header.stamp
frame_id = resp.image.header.frame_id
image_bgr = CvBridge().imgmsg_to_cv2(resp.image, desired_encoding="bgr8")
```

Use `stamp_msg` as the single source of truth for image/pose alignment.

## TF2 Alignment Pattern

The reconstruction agent should use the source-stamped TF, not the visualization
TF. Query:

```text
target frame: map
source frame: response.image.header.frame_id  # usually camera_frame
time: response.image.header.stamp
```

Example:

```python
import rclpy
from rclpy.duration import Duration
from rclpy.time import Time
from tf2_ros import Buffer, TransformListener

tf_buffer = Buffer()
tf_listener = TransformListener(tf_buffer, node)

stamp = Time.from_msg(resp.image.header.stamp)
source_frame = resp.image.header.frame_id or "camera_frame"

transform = tf_buffer.lookup_transform(
    "map",
    source_frame,
    stamp,
    timeout=Duration(seconds=1.0),
)
```

The transform is the SLAM-estimated pose of `camera_frame` in `map` at the image
timestamp. Use this transform with `image_bgr` for DDGS reconstruction.

## Handling SLAM Latency

The image service returns the latest available equirectangular frame. SLAM can
lag behind the camera stream, so the requested image timestamp can be slightly
newer than the latest TF sample.

Recommended behavior:

1. Request one image.
2. Try `lookup_transform("map", frame_id, image_stamp, timeout=1.0)`.
3. If TF2 reports extrapolation into the future, wait briefly and retry the same
   image stamp.
4. If the transform is still unavailable after a bounded timeout, skip that
   image and request another one.
5. If SLAM is lost, `/run_slam/camera_pose` and the source-stamped TF will stop
   advancing; skip frames until tracking recovers.

Do not use `map -> slam_camera` for strict temporal alignment when live
visualization is configured with `TF_STAMP_MODE=now`; it is intended for RViz
smoothness. Use `map -> camera_frame` for reconstruction.
