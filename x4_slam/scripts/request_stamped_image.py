#!/usr/bin/env python3
import argparse
from pathlib import Path

import cv2
import rclpy
from cv_bridge import CvBridge
from insta360_ros_driver.srv import GetStampedImage


def parse_args():
    parser = argparse.ArgumentParser(description="Request one stamped image from the Insta360 equirectangular service.")
    parser.add_argument("--service", default="/camera/get_stamped_image")
    parser.add_argument("--width", type=int, default=0)
    parser.add_argument("--height", type=int, default=0)
    parser.add_argument("--encoding", default="")
    parser.add_argument("--output", default="", help="Optional output image path, such as /tmp/frame.png")
    parser.add_argument("--timeout", type=float, default=5.0)
    return parser.parse_args()


def main():
    args = parse_args()
    rclpy.init()
    node = rclpy.create_node("request_stamped_image_client")
    client = node.create_client(GetStampedImage, args.service)

    try:
        if not client.wait_for_service(timeout_sec=args.timeout):
            raise RuntimeError(f"Service not available: {args.service}")

        request = GetStampedImage.Request()
        request.width = args.width
        request.height = args.height
        request.encoding = args.encoding

        future = client.call_async(request)
        rclpy.spin_until_future_complete(node, future, timeout_sec=args.timeout)
        if not future.done():
            raise RuntimeError(f"Service call timed out: {args.service}")

        response = future.result()
        if not response.success:
            raise RuntimeError(response.message)

        stamp = response.stamp
        image = response.image
        print(
            f"stamp={stamp.sec}.{stamp.nanosec:09d} "
            f"frame_id={response.frame_id} "
            f"size={image.width}x{image.height} "
            f"encoding={image.encoding}"
        )

        if args.output:
            bridge = CvBridge()
            cv_image = bridge.imgmsg_to_cv2(image, desired_encoding="passthrough")
            output = Path(args.output)
            output.parent.mkdir(parents=True, exist_ok=True)
            if not cv2.imwrite(str(output), cv_image):
                raise RuntimeError(f"Failed to write image: {output}")
            print(f"wrote={output}")
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
