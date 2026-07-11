#!/usr/bin/env python3
import os

import rclpy
from geometry_msgs.msg import TransformStamped
from nav_msgs.msg import Odometry
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from tf2_ros import TransformBroadcaster


class OdomToTf(Node):
    def __init__(self):
        super().__init__("odom_to_tf_bridge")
        self.odom_topic = os.environ.get("ODOM_TOPIC", "/run_slam/camera_pose")
        self.parent_frame = os.environ.get("TF_PARENT_FRAME", "map")
        self.child_frame = os.environ.get("TF_CHILD_FRAME", "slam_camera")
        self.source_child_frame = os.environ.get("TF_SOURCE_CHILD_FRAME", "")
        self.republish_hz = self._float_env("TF_REPUBLISH_HZ", 20.0)
        self.max_pose_age_sec = self._float_env("TF_MAX_POSE_AGE_SEC", 2.0)
        self.stamp_mode = os.environ.get("TF_STAMP_MODE", "now").lower()
        self.latest_msg = None
        self.last_received_time = None
        self.stale_warned = False
        self.br = TransformBroadcaster(self)
        self.sub = self.create_subscription(
            Odometry,
            self.odom_topic,
            self.callback,
            10,
        )
        if self.republish_hz > 0.0:
            self.timer = self.create_timer(1.0 / self.republish_hz, self.republish_latest)
        self.get_logger().info(
            f"Publishing TF {self.parent_frame} -> {self.child_frame} from "
            f"{self.odom_topic}; republish_hz={self.republish_hz:.1f}, "
            f"max_pose_age_sec={self.max_pose_age_sec:.1f}, stamp_mode={self.stamp_mode}"
        )
        if self.source_child_frame:
            self.get_logger().info(
                f"Publishing source-stamped TF {self.parent_frame} -> "
                f"{self.source_child_frame} from {self.odom_topic}"
            )

    def _float_env(self, name, default):
        value = os.environ.get(name)
        if value is None:
            return default
        try:
            return float(value)
        except ValueError:
            self.get_logger().warning(f"Invalid {name}={value!r}; using {default}")
            return default

    def build_transform(self, msg: Odometry, stamp=None, child_frame=None):
        transform = TransformStamped()
        transform.header.stamp = stamp or msg.header.stamp
        transform.header.frame_id = self.parent_frame or msg.header.frame_id or "map"
        transform.child_frame_id = child_frame or self.child_frame or msg.child_frame_id or "slam_camera"
        transform.transform.translation.x = msg.pose.pose.position.x
        transform.transform.translation.y = msg.pose.pose.position.y
        transform.transform.translation.z = msg.pose.pose.position.z
        transform.transform.rotation = msg.pose.pose.orientation
        return transform

    def publish_from_msg(self, msg: Odometry, stamp=None, child_frame=None):
        transform = self.build_transform(msg, stamp, child_frame)
        self.br.sendTransform(transform)

    def callback(self, msg: Odometry):
        self.latest_msg = msg
        self.last_received_time = self.get_clock().now()
        self.stale_warned = False
        self.publish_from_msg(msg)
        if self.source_child_frame:
            self.publish_from_msg(msg, msg.header.stamp, self.source_child_frame)

    def republish_latest(self):
        if self.latest_msg is None:
            return

        now = self.get_clock().now()
        if self.max_pose_age_sec >= 0.0 and self.last_received_time is not None:
            age_sec = (now - self.last_received_time).nanoseconds / 1e9
            if age_sec > self.max_pose_age_sec:
                if not self.stale_warned:
                    self.get_logger().warning(
                        f"No fresh pose from {self.odom_topic} for {age_sec:.2f}s; "
                        "stop republishing TF until a new pose arrives"
                    )
                    self.stale_warned = True
                return

        stamp = now.to_msg() if self.stamp_mode == "now" else self.latest_msg.header.stamp
        self.publish_from_msg(self.latest_msg, stamp)


def main():
    rclpy.init()
    node = OdomToTf()
    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
