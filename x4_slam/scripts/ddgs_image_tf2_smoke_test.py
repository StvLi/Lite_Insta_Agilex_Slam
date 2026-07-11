#!/usr/bin/env python3
import argparse
import math
from pathlib import Path

import cv2
import rclpy
from cv_bridge import CvBridge
from rclpy.duration import Duration
from rclpy.time import Time
from tf2_ros import Buffer, TransformException, TransformListener

from insta360_ros_driver.srv import GetStampedImage


def parse_args():
    parser = argparse.ArgumentParser(
        description="Request one stamped Insta360 image and align it with a TF2 camera pose."
    )
    parser.add_argument("--service", default="/camera/get_stamped_image")
    parser.add_argument("--target-frame", default="map")
    parser.add_argument("--source-frame", default="", help="Override response image frame_id.")
    parser.add_argument("--width", type=int, default=1440)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--encoding", default="bgr8")
    parser.add_argument("--output", default="/tmp/ddgs_stamped_image.png")
    parser.add_argument("--service-timeout", type=float, default=5.0)
    parser.add_argument("--tf-timeout", type=float, default=3.0)
    parser.add_argument("--tf-retry-period", type=float, default=0.1)
    return parser.parse_args()


def quaternion_to_matrix(x, y, z, w):
    norm = math.sqrt(x * x + y * y + z * z + w * w)
    if norm == 0.0:
        return [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
    x, y, z, w = x / norm, y / norm, z / norm, w / norm
    return [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ]


def print_transform(transform):
    t = transform.transform.translation
    q = transform.transform.rotation
    rot = quaternion_to_matrix(q.x, q.y, q.z, q.w)
    matrix = [
        [rot[0][0], rot[0][1], rot[0][2], t.x],
        [rot[1][0], rot[1][1], rot[1][2], t.y],
        [rot[2][0], rot[2][1], rot[2][2], t.z],
        [0.0, 0.0, 0.0, 1.0],
    ]

    print("translation_xyz: " f"{t.x:.6f}, {t.y:.6f}, {t.z:.6f}")
    print("quaternion_xyzw: " f"{q.x:.6f}, {q.y:.6f}, {q.z:.6f}, {q.w:.6f}")
    print("T_target_source:")
    for row in matrix:
        print("  " + " ".join(f"{value: .8f}" for value in row))


def main():
    args = parse_args()
    rclpy.init()
    node = rclpy.create_node("ddgs_image_tf2_smoke_test")
    bridge = CvBridge()
    tf_buffer = Buffer()
    TransformListener(tf_buffer, node)
    client = node.create_client(GetStampedImage, args.service)

    try:
        if not client.wait_for_service(timeout_sec=args.service_timeout):
            raise RuntimeError(f"Service not available: {args.service}")

        request = GetStampedImage.Request()
        request.width = args.width
        request.height = args.height
        request.encoding = args.encoding

        future = client.call_async(request)
        rclpy.spin_until_future_complete(node, future, timeout_sec=args.service_timeout)
        if not future.done():
            raise RuntimeError(f"Service call timed out: {args.service}")

        response = future.result()
        if response is None or not response.success:
            message = response.message if response else "empty response"
            raise RuntimeError(f"Image request failed: {message}")

        stamp = Time.from_msg(response.image.header.stamp)
        stamp_msg = response.image.header.stamp
        source_frame = args.source_frame or response.image.header.frame_id or "camera_frame"

        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        cv_image = bridge.imgmsg_to_cv2(response.image, desired_encoding="passthrough")
        if not cv2.imwrite(str(output), cv_image):
            raise RuntimeError(f"Failed to write image: {output}")

        deadline = node.get_clock().now() + Duration(seconds=args.tf_timeout)
        transform = None
        last_error = None
        while node.get_clock().now() < deadline:
            try:
                transform = tf_buffer.lookup_transform(
                    args.target_frame,
                    source_frame,
                    stamp,
                    timeout=Duration(seconds=args.tf_retry_period),
                )
                break
            except TransformException as exc:
                last_error = exc
                rclpy.spin_once(node, timeout_sec=args.tf_retry_period)

        if transform is None:
            raise RuntimeError(
                f"No TF for {args.target_frame} <- {source_frame} at "
                f"{stamp_msg.sec}.{stamp_msg.nanosec:09d}: {last_error}"
            )

        print("image_request: ok")
        print(f"image_stamp: {stamp_msg.sec}.{stamp_msg.nanosec:09d}")
        print(f"image_frame_id: {response.image.header.frame_id}")
        print(f"image_size: {response.image.width}x{response.image.height}")
        print(f"image_encoding: {response.image.encoding}")
        print(f"image_file: {output}")
        print(
            "tf_stamp: "
            f"{transform.header.stamp.sec}.{transform.header.stamp.nanosec:09d}"
        )
        print(f"tf_frames: {args.target_frame} <- {source_frame}")
        print_transform(transform)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
