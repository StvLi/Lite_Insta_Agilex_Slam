# Troubleshooting

## CameraSDK sees `no device found`

First check the camera mode:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_camera_usb.sh
```

For the SDK route, the X4/X5 must be in Android mode. In this mode it should appear as a vendor-specific USB device, for example:

```text
ID 2e1a:0002 Arashi Vision Insta360 X5
Class=Vendor Specific Class
```

If the camera appears as `/dev/video0`, it is in UVC/webcam mode. That mode is useful for generic webcam capture but not for the official CameraSDK path.

If the device is in Android mode but `CameraSDKTest` still reports `no device found`, check the USB node permissions:

```bash
ls -l /dev/bus/usb/001/004
```

The SDK uses libusb and usually needs write access to the USB node. A practical udev rule is:

```text
SUBSYSTEM=="usb", ATTR{idVendor}=="2e1a", MODE="0666", TAG+="uaccess"
```

Install it as root:

```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="2e1a", MODE="0666", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/99-insta360.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then unplug/replug the camera, choose Android mode again, and rerun:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_sdk_demo.sh
```

## Stella VSLAM publishes no pose

First confirm that images are flowing:

```bash
ros2 topic hz /camera/image_raw
ros2 topic echo --once /camera/image_raw/header
```

Then start SLAM through the project script:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/run_slam.sh
```

In another terminal:

```bash
/home/deep/peize/where_is_my_key/Lite_Insta_Agilex_Slam/x4_slam/scripts/check_slam_outputs.sh
```

If `/camera/image_raw` is alive but `/run_slam/camera_pose` stays silent, read the SLAM terminal logs:

- Repeated `try to initialize` messages usually mean monocular initialization has not found enough parallax.
- `empty map` on shutdown means no valid keyframes were created.
- A very small `data/maps/stella_*.msg` file is also a sign of an empty map.

For the next bag, use a textured scene and include slow sideways/forward translation. Pure panoramic rotation is useful for visual inspection but weak for monocular SLAM initialization.
