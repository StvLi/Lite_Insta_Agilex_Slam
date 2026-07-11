#ifndef EQUIRECTANGULAR_HPP
#define EQUIRECTANGULAR_HPP

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>
#include <insta360_ros_driver/srv/get_stamped_image.hpp>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>

class EquirectangularNode : public rclcpp::Node
{
public:
    explicit EquirectangularNode();
    ~EquirectangularNode();

private:
    // Callback functions
    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    void getStampedImageCallback(
        const std::shared_ptr<insta360_ros_driver::srv::GetStampedImage::Request> request,
        std::shared_ptr<insta360_ros_driver::srv::GetStampedImage::Response> response);
    rcl_interfaces::msg::SetParametersResult parametersCallback(const std::vector<rclcpp::Parameter> &parameters);
    
    // Initialization functions
    void loadParameters();
    void updateCameraParameters();
    void initMapping(int img_height, int img_width);
    
    // Processing functions
    cv::Mat createEquirectangular(const cv::Mat& front_img, const cv::Mat& back_img);
    bool buildServiceImage(
        const sensor_msgs::msg::Image& source_image,
        const std::string& requested_encoding,
        uint32_t requested_width,
        uint32_t requested_height,
        sensor_msgs::msg::Image& output_image,
        std::string& message) const;
    
    // ROS2 communication
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr dual_fisheye_sub_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr equirect_pub_;
    rclcpp::Service<insta360_ros_driver::srv::GetStampedImage>::SharedPtr image_service_;
    
    // Parameters
    double cx_offset_;
    double cy_offset_;
    int crop_size_;
    double tx_, ty_, tz_;
    double roll_, pitch_, yaw_;
    bool gpu_enabled_;
    int out_width_;
    int out_height_;
    
    // Camera parameters
    double cx_, cy_;
    cv::Mat back_to_front_rotation_;
    cv::Vec3d back_to_front_translation_;
    
    // Mapping matrices
    cv::Mat front_map_x_, front_map_y_;
    cv::Mat back_map_x_, back_map_y_;
    cv::Mat front_mask_, back_mask_;
    
    // State management
    std::atomic<bool> maps_initialized_;
    std::atomic<bool> params_changed_;
    int img_height_;
    int img_width_;
    
    // Thread safety
    std::mutex processing_mutex_;
    std::mutex latest_image_mutex_;
    sensor_msgs::msg::Image::SharedPtr latest_equirect_image_;
};

#endif // EQUIRECTANGULAR_HPP
