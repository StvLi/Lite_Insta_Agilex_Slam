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
        self.br = TransformBroadcaster(self)
        self.sub = self.create_subscription(
            Odometry,
            self.odom_topic,
            self.callback,
            10,
        )
        self.get_logger().info(
            f"Publishing TF {self.parent_frame} -> {self.child_frame} from {self.odom_topic}"
        )

    def callback(self, msg: Odometry):
        transform = TransformStamped()
        transform.header.stamp = msg.header.stamp
        transform.header.frame_id = self.parent_frame or msg.header.frame_id or "map"
        transform.child_frame_id = self.child_frame or msg.child_frame_id or "slam_camera"
        transform.transform.translation.x = msg.pose.pose.position.x
        transform.transform.translation.y = msg.pose.pose.position.y
        transform.transform.translation.z = msg.pose.pose.position.z
        transform.transform.rotation = msg.pose.pose.orientation
        self.br.sendTransform(transform)


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
