#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <atomic>
#include <ctime>
#include <tuple>

#include <camera/camera.h>
#include <camera/photography_settings.h>
#include <camera/device_discovery.h>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/qos.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "sensor_msgs/msg/imu.hpp"

namespace {

bool parseResolution(
    const std::string& value,
    ins_camera::VideoResolution& resolution,
    int& width,
    int& height)
{
    static const std::vector<std::tuple<std::string, ins_camera::VideoResolution, int, int>> resolutions = {
        {"RES_3840_1920P30", ins_camera::VideoResolution::RES_3840_1920P30, 3840, 1920},
        {"3840x1920", ins_camera::VideoResolution::RES_3840_1920P30, 3840, 1920},
        {"RES_2560_1280P30", ins_camera::VideoResolution::RES_2560_1280P30, 2560, 1280},
        {"2560x1280", ins_camera::VideoResolution::RES_2560_1280P30, 2560, 1280},
        {"RES_1920_960P30", ins_camera::VideoResolution::RES_1920_960P30, 1920, 960},
        {"1920x960", ins_camera::VideoResolution::RES_1920_960P30, 1920, 960},
        {"RES_1440_720P30", ins_camera::VideoResolution::RES_1440_720P30, 1440, 720},
        {"1440x720", ins_camera::VideoResolution::RES_1440_720P30, 1440, 720},
        {"RES_1024_512P30", ins_camera::VideoResolution::RES_1024_512P30, 1024, 512},
        {"1024x512", ins_camera::VideoResolution::RES_1024_512P30, 1024, 512},
    };

    for (const auto& [name, candidate, candidate_width, candidate_height] : resolutions) {
        if (value == name) {
            resolution = candidate;
            width = candidate_width;
            height = candidate_height;
            return true;
        }
    }
    return false;
}

}  // namespace

class TestStreamDelegate : public ins_camera::StreamDelegate {
private:
    std::shared_ptr<rclcpp::Node> node_;
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_pub_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;

public:
    TestStreamDelegate(const std::shared_ptr<rclcpp::Node>& node) : node_(node) {
        // Publisher for the compressed H.264 video stream
        compressed_pub_ = node_->create_publisher<sensor_msgs::msg::CompressedImage>(
            "/dual_fisheye/image/compressed", 
            rclcpp::QoS(10)
        );

        // Publisher for IMU data (remains the same)
        imu_pub_ = node_->create_publisher<sensor_msgs::msg::Imu>("imu/data_raw", rclcpp::SensorDataQoS());
        RCLCPP_INFO(node_->get_logger(), "Publisher for compressed images and IMU created.");
    }

    virtual ~TestStreamDelegate() {}

    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {
        (void)data;
        (void)size;
        (void)timestamp;
    }

    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index) override {
        (void)timestamp;
        (void)streamType;

        // We only care about the main video stream (index 0)
        if (stream_index == 0 && size > 0 && compressed_pub_) {
            auto msg = std::make_unique<sensor_msgs::msg::CompressedImage>();

            // Set the header
            msg->header.stamp = node_->get_clock()->now();
            msg->header.frame_id = "camera_frame";

            // Set the format to H.264
            // The subscriber will need to know this to select the correct decoder.
            msg->format = "h264";

            // Copy the compressed video data directly into the message
            msg->data.assign(data, data + size);

            compressed_pub_->publish(std::move(msg));
        }
    }

    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {
        for (const auto& gyro : data) {
            auto msg = std::make_unique<sensor_msgs::msg::Imu>();
            msg->header.stamp = node_->get_clock()->now();
            msg->header.frame_id = "imu_frame";
            msg->angular_velocity.x = gyro.gx;
            msg->angular_velocity.y = gyro.gy;
            msg->angular_velocity.z = gyro.gz;
            
            msg->linear_acceleration.x = gyro.ax * 9.80665;
            msg->linear_acceleration.y = gyro.ay * 9.80665;
            msg->linear_acceleration.z = gyro.az * 9.80665;

            msg->orientation.x = 0.0;
            msg->orientation.y = 0.0;
            msg->orientation.z = 0.0;
            msg->orientation.w = 1.0; // Neutral orientation
            msg->orientation_covariance[0] = -1.0; // No orientation data available

            for (int i = 0; i < 9; i++)
            {
                msg->angular_velocity_covariance[i] = 0;
                msg->linear_acceleration_covariance[i] = 0;
            }
            imu_pub_->publish(std::move(msg));
        }
    }

    void OnExposureData(const ins_camera::ExposureData& data) override {
        (void)data;
    }
};

class CameraWrapper {
private:
    std::shared_ptr<ins_camera::Camera> cam;
    std::shared_ptr<rclcpp::Node> node_;

public:
    CameraWrapper(const std::shared_ptr<rclcpp::Node>& node) : node_(node) {}

    ~CameraWrapper() {
        if (cam) {
            cam->Close();
        }
    }

    int run_camera() {
        ins_camera::DeviceDiscovery discovery;
        auto list = discovery.GetAvailableDevices();
        if (list.empty()) {
            RCLCPP_ERROR(node_->get_logger(), "No available camera devices found.");
            return -1;
        }

        const auto camera_type = list[0].camera_type;
        RCLCPP_INFO(
            node_->get_logger(),
            "Using camera: serial=%s name=%s fw=%s",
            list[0].serial_number.c_str(),
            list[0].camera_name.c_str(),
            list[0].fw_version.c_str());

        cam = std::make_shared<ins_camera::Camera>(list[0].info);
        if (!cam->Open()) {
            RCLCPP_ERROR(node_->get_logger(), "Failed to open camera.");
            return -1;
        }
        RCLCPP_INFO(node_->get_logger(), "Camera opened successfully.");
        discovery.FreeDeviceDescriptors(list);

        std::shared_ptr<ins_camera::StreamDelegate> delegate = std::make_shared<TestStreamDelegate>(node_);
        cam->SetStreamDelegate(delegate);

        const auto start = static_cast<uint64_t>(time(nullptr));
        cam->SyncLocalTimeToCamera(start, 0);

        const auto resolution_param = node_->declare_parameter<std::string>("stream_resolution", "RES_1920_960P30");
        const int bitrate_mbps = node_->declare_parameter<int>("stream_bitrate_mbps", 6);

        ins_camera::VideoResolution resolution;
        int resolution_width = 0;
        int resolution_height = 0;
        if (!parseResolution(resolution_param, resolution, resolution_width, resolution_height)) {
            RCLCPP_ERROR(
                node_->get_logger(),
                "Unsupported stream_resolution '%s'. Try RES_1920_960P30, RES_2560_1280P30, or RES_3840_1920P30.",
                resolution_param.c_str());
            return -1;
        }

        RCLCPP_INFO(
            node_->get_logger(),
            "Configuring live stream: resolution=%s (%dx%d), bitrate=%d Mbps",
            resolution_param.c_str(),
            resolution_width,
            resolution_height,
            bitrate_mbps);

        if (camera_type >= ins_camera::CameraType::Insta360X4) {
            if (!cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_LIVEVIEW)) {
                RCLCPP_ERROR(node_->get_logger(), "Failed to switch camera to live-view video submode.");
                return -1;
            }

            ins_camera::RecordParams record_params;
            record_params.resolution = resolution;
            record_params.bitrate = 0;
            if (!cam->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_LIVE_STREAM)) {
                RCLCPP_ERROR(node_->get_logger(), "Failed to configure live stream capture parameters.");
                return -1;
            }
        }

        ins_camera::LiveStreamParam param;
        param.video_resolution = resolution;
        param.lrv_video_resulution = ins_camera::VideoResolution::RES_1440_720P30;
        param.video_bitrate = 1024 * 1024 * bitrate_mbps;
        param.enable_audio = false;
        param.enable_gyro = true;
        param.using_lrv = false;

        if (!cam->StartLiveStreaming(param)) {
            RCLCPP_ERROR(node_->get_logger(), "Failed to start live streaming.");
            return -1;
        }
        
        RCLCPP_INFO(node_->get_logger(), "Live streaming started.");
        return 0;
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("insta_publisher");
    
    CameraWrapper camera(node);
    if (camera.run_camera() != 0) {
        rclcpp::shutdown();
        return -1;
    }
    
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
