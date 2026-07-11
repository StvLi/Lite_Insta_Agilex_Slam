#include "equirectangular.hpp"
#include <rcl_interfaces/msg/set_parameters_result.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <thread>
#include <chrono>

namespace
{
std::string normalizeEncoding(const std::string& encoding)
{
    std::string normalized = encoding;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized;
}
}

EquirectangularNode::EquirectangularNode()
    : Node("equirectangular_node"),
      maps_initialized_(false),
      params_changed_(true),
      img_height_(0),
      img_width_(0)
{
    // Declare parameters
    declare_parameter("cx_offset", 0.0);
    declare_parameter("cy_offset", 0.0);
    declare_parameter("crop_size", 960);
    declare_parameter("translation", std::vector<double>{0.0, 0.0, -0.105});
    declare_parameter("rotation_deg", std::vector<double>{-0.5, 0.0, 1.1});
    declare_parameter("gpu", true);
    declare_parameter("out_width", 1920);
    declare_parameter("out_height", 960);
    
    // Load parameters
    loadParameters();
    
    // Log GPU settings (note: C++ version currently only supports CPU)
    RCLCPP_INFO(get_logger(), "C++ equirectangular node");
    
    
    // Add parameter callback
    auto params_callback_handle = add_on_set_parameters_callback(
        std::bind(&EquirectangularNode::parametersCallback, this, std::placeholders::_1));
    
    updateCameraParameters();
    
    // Configure QoS
    auto qos = rclcpp::QoS(1).reliable();
    
    // Create publishers and subscribers
    dual_fisheye_sub_ = create_subscription<sensor_msgs::msg::Image>(
        "/dual_fisheye/image", qos,
        std::bind(&EquirectangularNode::imageCallback, this, std::placeholders::_1));
    
    equirect_pub_ = create_publisher<sensor_msgs::msg::Image>(
        "/equirectangular/image", qos);

    image_service_ = create_service<insta360_ros_driver::srv::GetStampedImage>(
        "/camera/get_stamped_image",
        std::bind(&EquirectangularNode::getStampedImageCallback, this,
                  std::placeholders::_1, std::placeholders::_2));
    RCLCPP_INFO(get_logger(), "Image snapshot service: /camera/get_stamped_image");
}

EquirectangularNode::~EquirectangularNode()
{
}

void EquirectangularNode::loadParameters()
{
    try {
        cx_offset_ = get_parameter("cx_offset").as_double();
        cy_offset_ = get_parameter("cy_offset").as_double();
        crop_size_ = get_parameter("crop_size").as_int();
        out_width_ = get_parameter("out_width").as_int();
        out_height_ = get_parameter("out_height").as_int();
        gpu_enabled_ = get_parameter("gpu").as_bool();
        
        auto translation = get_parameter("translation").as_double_array();
        tx_ = translation[0];
        ty_ = translation[1];
        tz_ = translation[2];
        
        auto rotation_deg = get_parameter("rotation_deg").as_double_array();
        roll_ = rotation_deg[0] * M_PI / 180.0;
        pitch_ = rotation_deg[1] * M_PI / 180.0;
        yaw_ = rotation_deg[2] * M_PI / 180.0;
        
        RCLCPP_INFO(get_logger(), "Loaded parameters from ROS parameter server");
        RCLCPP_INFO(get_logger(), "  Crop size: %d", crop_size_);
        RCLCPP_INFO(get_logger(), "  Center offset: (%.1f, %.1f)", cx_offset_, cy_offset_);
        RCLCPP_INFO(get_logger(), "  Translation: [%.3f, %.3f, %.3f]", tx_, ty_, tz_);
        RCLCPP_INFO(get_logger(), "  Rotation (deg): [%.1f, %.1f, %.1f]", 
                    rotation_deg[0], rotation_deg[1], rotation_deg[2]);
        RCLCPP_INFO(get_logger(), "  Output size: %dx%d", out_width_, out_height_);
        RCLCPP_INFO(get_logger(), "  GPU enabled: %s", gpu_enabled_ ? "true" : "false");
    } catch (const std::exception& e) {
        RCLCPP_ERROR(get_logger(), "Error loading parameters: %s", e.what());
        gpu_enabled_ = true;
        throw;
    }
}

void EquirectangularNode::updateCameraParameters()
{
    // Build rotation matrix
    cv::Mat Rx = (cv::Mat_<double>(3, 3) <<
        1.0, 0.0, 0.0,
        0.0, cos(roll_), -sin(roll_),
        0.0, sin(roll_), cos(roll_));
    
    cv::Mat Ry = (cv::Mat_<double>(3, 3) <<
        cos(pitch_), 0.0, sin(pitch_),
        0.0, 1.0, 0.0,
        -sin(pitch_), 0.0, cos(pitch_));
    
    cv::Mat Rz = (cv::Mat_<double>(3, 3) <<
        cos(yaw_), -sin(yaw_), 0.0,
        sin(yaw_), cos(yaw_), 0.0,
        0.0, 0.0, 1.0);
    
    back_to_front_rotation_ = Rz * Ry * Rx;
    back_to_front_translation_ = cv::Vec3d(tx_, ty_, tz_);
    
    if (maps_initialized_) {
        maps_initialized_ = false;
        RCLCPP_INFO(get_logger(), "Parameters updated, remapping will occur on next image");
    }
}

void EquirectangularNode::initMapping(int img_height, int img_width)
{
    RCLCPP_INFO(get_logger(), "Initializing equirectangular projection: fusing two %dx%d fisheye images to %dx%d",
                img_width, img_height, out_width_, out_height_);
    
    img_height_ = img_height;
    img_width_ = img_width;
    
    cx_ = img_width / 2.0 + cx_offset_;
    cy_ = img_height / 2.0 + cy_offset_;
    
    // Create output coordinate grids
    cv::Mat x_grid, y_grid;
    cv::Mat x_range = cv::Mat::zeros(1, out_width_, CV_32F);
    cv::Mat y_range = cv::Mat::zeros(out_height_, 1, CV_32F);
    
    for (int i = 0; i < out_width_; ++i) {
        x_range.at<float>(0, i) = static_cast<float>(i);
    }
    for (int i = 0; i < out_height_; ++i) {
        y_range.at<float>(i, 0) = static_cast<float>(i);
    }
    
    cv::repeat(x_range, out_height_, 1, x_grid);
    cv::repeat(y_range, 1, out_width_, y_grid);
    
    // Convert to spherical coordinates
    // Note: x=0 corresponds to lon=-π, x=out_width-1 corresponds to lon=π*(out_width-1)/out_width
    cv::Mat longitude = (x_grid / (float)out_width_) * 2 * M_PI - M_PI;
    cv::Mat latitude = (y_grid / (float)out_height_) * M_PI - M_PI / 2;
    
    cv::Mat X, Y, Z;
    cv::Mat cos_lat, sin_lat, cos_lon, sin_lon;
    
    cv::exp(-latitude, cos_lat); // Using exp(-x) as intermediate for cos calculation
    cos_lat = (1 - cos_lat) / (1 + cos_lat); // Convert to cos
    cv::sqrt(1 - cos_lat.mul(cos_lat), sin_lat);
    
    cv::exp(-longitude, cos_lon);
    cos_lon = (1 - cos_lon) / (1 + cos_lon);
    cv::sqrt(1 - cos_lon.mul(cos_lon), sin_lon);
    
    // Correct calculation
    cv::Mat cos_latitude, sin_latitude;
    for (int y = 0; y < out_height_; ++y) {
        for (int x = 0; x < out_width_; ++x) {
            float lat = latitude.at<float>(y, x);
            float lon = longitude.at<float>(y, x);
            cos_lat.at<float>(y, x) = cos(lat);
            sin_lat.at<float>(y, x) = sin(lat);
            cos_lon.at<float>(y, x) = cos(lon);
            sin_lon.at<float>(y, x) = sin(lon);
        }
    }
    
    X = cos_lat.mul(sin_lon);
    Y = sin_lat;
    Z = cos_lat.mul(cos_lon);
    
    // Create masks
    front_mask_ = Z >= 0;
    back_mask_ = Z < 0;
    
    // Initialize mapping matrices
    front_map_x_ = cv::Mat::zeros(out_height_, out_width_, CV_32F);
    front_map_y_ = cv::Mat::zeros(out_height_, out_width_, CV_32F);
    back_map_x_ = cv::Mat::zeros(out_height_, out_width_, CV_32F);
    back_map_y_ = cv::Mat::zeros(out_height_, out_width_, CV_32F);
    
    // Process front hemisphere
    for (int y = 0; y < out_height_; ++y) {
        for (int x = 0; x < out_width_; ++x) {
            if (front_mask_.at<uchar>(y, x)) {
                float X_val = X.at<float>(y, x);
                float Y_val = Y.at<float>(y, x);
                float Z_val = Z.at<float>(y, x);
                
                float r = sqrt(X_val * X_val + Y_val * Y_val);
                if (r < 1e-6) r = 1e-6;
                
                float theta = atan2(r, fabs(Z_val));
                float r_fisheye = 2 * theta / M_PI * (img_width / 2.0);
                
                front_map_x_.at<float>(y, x) = cx_ + X_val / r * r_fisheye;
                front_map_y_.at<float>(y, x) = cy_ + Y_val / r * r_fisheye;
            }
        }
    }
    
    // Process back hemisphere
    for (int y = 0; y < out_height_; ++y) {
        for (int x = 0; x < out_width_; ++x) {
            if (back_mask_.at<uchar>(y, x)) {
                cv::Vec3d point(X.at<float>(y, x), Y.at<float>(y, x), Z.at<float>(y, x));
                
                // Transform point
                cv::Mat point_mat = cv::Mat(point);
                cv::Mat transformed = back_to_front_rotation_ * point_mat + cv::Mat(back_to_front_translation_);
                
                float X_back = -transformed.at<double>(0);
                float Y_back = transformed.at<double>(1);
                float Z_back = transformed.at<double>(2);
                
                float r = sqrt(X_back * X_back + Y_back * Y_back);
                if (r < 1e-6) r = 1e-6;
                
                float theta = atan2(r, fabs(Z_back));
                float r_fisheye = 2 * theta / M_PI * (img_width / 2.0);
                
                back_map_x_.at<float>(y, x) = cx_ + X_back / r * r_fisheye;
                back_map_y_.at<float>(y, x) = cy_ + Y_back / r * r_fisheye;
            }
        }
    }
    
    maps_initialized_ = true;
    
    RCLCPP_INFO(get_logger(), "Mapping matrices initialization complete");
}

cv::Mat EquirectangularNode::createEquirectangular(const cv::Mat& front_img, const cv::Mat& back_img)
{
    if (!maps_initialized_ || params_changed_ ||
        front_img.rows != img_height_ || front_img.cols != img_width_) {
        initMapping(front_img.rows, front_img.cols);
        params_changed_ = false;
    }
    
    if (!maps_initialized_) {
        RCLCPP_ERROR(get_logger(), "Mapping arrays not properly initialized");
        return cv::Mat::zeros(out_height_, out_width_, CV_8UC3);
    }
    
    cv::Mat front_result, back_result;
    cv::remap(front_img, front_result, front_map_x_, front_map_y_, cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::remap(back_img, back_result, back_map_x_, back_map_y_, cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    
    cv::Mat equirect = cv::Mat::zeros(out_height_, out_width_, CV_8UC3);
    
    // Apply masks
    cv::Mat front = front_mask_;
    cv::Mat back  = ~front_mask_;
    
    front_result.copyTo(equirect, front);
    back_result.copyTo(equirect, back);

    return equirect;
}

bool EquirectangularNode::buildServiceImage(
    const sensor_msgs::msg::Image& source_image,
    const std::string& requested_encoding,
    uint32_t requested_width,
    uint32_t requested_height,
    sensor_msgs::msg::Image& output_image,
    std::string& message) const
{
    uint32_t target_width = requested_width;
    uint32_t target_height = requested_height;

    if (target_width == 0 && target_height == 0) {
        target_width = source_image.width;
        target_height = source_image.height;
    } else if (target_width == 0) {
        target_width = static_cast<uint32_t>(
            std::max(1.0, std::round(static_cast<double>(source_image.width) *
                                     static_cast<double>(target_height) /
                                     static_cast<double>(source_image.height))));
    } else if (target_height == 0) {
        target_height = static_cast<uint32_t>(
            std::max(1.0, std::round(static_cast<double>(source_image.height) *
                                     static_cast<double>(target_width) /
                                     static_cast<double>(source_image.width))));
    }

    std::string target_encoding = normalizeEncoding(requested_encoding);
    if (target_encoding.empty() || target_encoding == "passthrough" || target_encoding == "source") {
        target_encoding = normalizeEncoding(source_image.encoding);
    }

    try {
        cv_bridge::CvImagePtr source_ptr = cv_bridge::toCvCopy(source_image, "bgr8");
        cv::Mat converted;
        std::string output_encoding;

        if (target_encoding == "bgr8") {
            converted = source_ptr->image;
            output_encoding = "bgr8";
        } else if (target_encoding == "rgb8") {
            cv::cvtColor(source_ptr->image, converted, cv::COLOR_BGR2RGB);
            output_encoding = "rgb8";
        } else if (target_encoding == "mono8") {
            cv::cvtColor(source_ptr->image, converted, cv::COLOR_BGR2GRAY);
            output_encoding = "mono8";
        } else {
            message = "Unsupported encoding '" + requested_encoding +
                      "'. Supported encodings: bgr8, rgb8, mono8, passthrough/source.";
            return false;
        }

        cv::Mat resized;
        if (converted.cols != static_cast<int>(target_width) ||
            converted.rows != static_cast<int>(target_height)) {
            cv::resize(converted, resized, cv::Size(target_width, target_height), 0, 0, cv::INTER_AREA);
        } else {
            resized = converted;
        }

        cv_bridge::CvImage response_image;
        response_image.header = source_image.header;
        response_image.encoding = output_encoding;
        response_image.image = resized;
        response_image.toImageMsg(output_image);

        std::ostringstream ok_message;
        ok_message << "Returning " << output_image.width << "x" << output_image.height
                   << " " << output_image.encoding << " image";
        message = ok_message.str();
        return true;
    } catch (const std::exception& e) {
        message = std::string("Failed to build service image: ") + e.what();
        return false;
    }
}

void EquirectangularNode::getStampedImageCallback(
    const std::shared_ptr<insta360_ros_driver::srv::GetStampedImage::Request> request,
    std::shared_ptr<insta360_ros_driver::srv::GetStampedImage::Response> response)
{
    sensor_msgs::msg::Image::SharedPtr latest_image;
    {
        std::lock_guard<std::mutex> lock(latest_image_mutex_);
        latest_image = latest_equirect_image_;
    }

    if (!latest_image) {
        response->success = false;
        response->message = "No equirectangular image is available yet";
        return;
    }

    sensor_msgs::msg::Image service_image;
    std::string message;
    if (!buildServiceImage(*latest_image, request->encoding, request->width, request->height,
                           service_image, message)) {
        response->success = false;
        response->message = message;
        response->stamp = latest_image->header.stamp;
        response->frame_id = latest_image->header.frame_id;
        return;
    }

    response->success = true;
    response->message = message;
    response->stamp = service_image.header.stamp;
    response->frame_id = service_image.header.frame_id;
    response->image = service_image;
}

void EquirectangularNode::imageCallback(const sensor_msgs::msg::Image::SharedPtr dual_fisheye_msg)
{
    
    try {
        cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(dual_fisheye_msg, "bgr8");
        cv::Mat dual_fisheye_img = cv_ptr->image;
        
        int img_height = dual_fisheye_img.rows;
        int img_width_full = dual_fisheye_img.cols;
        int midpoint = img_width_full / 2;
        
        cv::Mat front_img_full = dual_fisheye_img(cv::Rect(midpoint, 0, midpoint, img_height));
        cv::Mat back_img_full = dual_fisheye_img(cv::Rect(0, 0, midpoint, img_height));
        
        // cv::rotate(front_img_full, front_img_full, cv::ROTATE_90_COUNTERCLOCKWISE);
        // cv::rotate(back_img_full, back_img_full, cv::ROTATE_90_CLOCKWISE);
        
        
        // Crop images based on crop_size parameter
        cv::Mat front_img, back_img;
        int current_crop_size = crop_size_;
        int orig_h = front_img_full.rows;
        int orig_w = front_img_full.cols;
        
        if (orig_h != current_crop_size || orig_w != current_crop_size) {
            int y_start = (orig_h - current_crop_size) / 2;
            int x_start = (orig_w - current_crop_size) / 2;
            
            if (y_start >= 0 && x_start >= 0 &&
                y_start + current_crop_size <= orig_h &&
                x_start + current_crop_size <= orig_w) {
                front_img = front_img_full(cv::Rect(x_start, y_start, current_crop_size, current_crop_size));
                back_img = back_img_full(cv::Rect(x_start, y_start, current_crop_size, current_crop_size));
            } else {
                front_img = front_img_full;
                back_img = back_img_full;
            }
        } else {
            front_img = front_img_full;
            back_img = back_img_full;
        }
        
        
        // Initialize mapping if needed
        if (!maps_initialized_ || params_changed_ ||
            front_img.rows != img_height_ || front_img.cols != img_width_) {
            initMapping(front_img.rows, front_img.cols);
            params_changed_ = false;
        }
        
        auto start_time = now();
        cv::Mat equirect_img = createEquirectangular(front_img, back_img);
        
        // Publish result
        cv_bridge::CvImage out_msg;
        out_msg.header = dual_fisheye_msg->header;
        out_msg.encoding = "bgr8";
        out_msg.image = equirect_img;
        RCLCPP_INFO_ONCE(get_logger(), "Output image size: %dx%d", equirect_img.cols, equirect_img.rows);
        auto image_msg = out_msg.toImageMsg();
        equirect_pub_->publish(*image_msg);
        {
            std::lock_guard<std::mutex> lock(latest_image_mutex_);
            latest_equirect_image_ = image_msg;
        }
        
        auto process_time = (now() - start_time).seconds();
        RCLCPP_DEBUG(get_logger(), "Processing time: %.3f seconds", process_time);
        
    } catch (const cv_bridge::Exception& e) {
        RCLCPP_ERROR(get_logger(), "cv_bridge exception: %s", e.what());
    } catch (const std::exception& e) {
        RCLCPP_ERROR(get_logger(), "Error processing images: %s", e.what());
    }
}

rcl_interfaces::msg::SetParametersResult EquirectangularNode::parametersCallback(
    const std::vector<rclcpp::Parameter> &parameters)
{
    bool update_needed = false;
    
    for (const auto& param : parameters) {
        if (param.get_name() == "cx_offset" ||
            param.get_name() == "cy_offset" ||
            param.get_name() == "crop_size" ||
            param.get_name() == "translation" ||
            param.get_name() == "rotation_deg" ||
            param.get_name() == "out_width" ||
            param.get_name() == "out_height" ||
            param.get_name() == "gpu") {
            update_needed = true;
        }
    }
    
    if (update_needed) {
        loadParameters();
        updateCameraParameters();
    }
    
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    return result;
}


int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    
    auto node = std::make_shared<EquirectangularNode>();
    
    try {
        rclcpp::spin(node);
    } catch (const std::exception& e) {
        RCLCPP_ERROR(node->get_logger(), "Exception during spin: %s", e.what());
    }
    
    rclcpp::shutdown();
    return 0;
}
