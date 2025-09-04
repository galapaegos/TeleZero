#ifndef _camera_h_
#define _camera_h_

#undef signals
#undef slots
#undef emit
#undef foreach

#include <cstring>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <sys/mman.h>

#include <libcamera/libcamera.h>
#include <libcamera/formats.h>

class Camera {
  public:
    Camera();
    ~Camera();

    bool initialize();
    bool uninitialize();

    std::vector<std::string> get_cameras() const;
    bool connect_camera(const std::string &camera_name);
    bool disconnect_camera();

    bool configure_camera(const int &format_index, const int &size_index);

    bool start_camera();
    bool stop_camera();

    bool is_connected() const { return has_camera; }

    bool get_image(std::vector<uint8_t> &frame_buffer);

    std::vector<std::string> get_pixel_formats() const;
    std::vector<std::string> get_pixel_format_sizes(const std::string &format);

    int width;
    int height;
    int channels;

    int lines_per_row;
    int padding;

    int64_t sequence;

    float analogue_gain;
    int32_t exposure_time;

    float brightness;
    float contrast;
    float saturation;
    float lens_position;
    float temperature;

  private:
    int get_channels(const libcamera::PixelFormat &format);
    int queue_request(libcamera::Request *request);
    void process_request(libcamera::Request *request);
    void request_complete(libcamera::Request *request);

    bool has_camera;
    bool camera_started;

    std::vector<libcamera::PixelFormat> pixel_formats;
    std::map<libcamera::PixelFormat, std::vector<libcamera::Size>> pixel_format_sizes;

    libcamera::Stream *stream;

    std::shared_ptr<libcamera::Camera> camera;
    std::unique_ptr<libcamera::CameraManager> camera_manager;

    std::unique_ptr<libcamera::CameraConfiguration> config;
    std::unique_ptr<libcamera::FrameBufferAllocator> allocator;
    std::vector<std::unique_ptr<libcamera::Request>> requests;
    std::map<int, std::pair<void *, unsigned int>> mapped_buffers;

    std::queue<libcamera::Request *> request_queue;

    std::vector<libcamera::PixelFormat> supported_formats;

    libcamera::ControlList cam_controls;
    std::mutex control_mutex;
    std::mutex camera_stop_mutex;
    std::mutex free_requests_mutex;

    std::vector<uint8_t> image_buffer;

    libcamera::StreamConfiguration stream_config;
};

#endif
