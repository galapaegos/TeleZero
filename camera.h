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
#include <sys/mman.h>

#include <libcamera/libcamera.h>
#include <libcamera/formats.h>

std::string convert_format(const libcamera::PixelFormat &format);
libcamera::PixelFormat convert_format(const std::string &format);

class Camera
{
public:
    Camera();
    ~Camera();
	
	bool initialize();
	bool uninitialize();
	
	std::vector<std::string> get_cameras() const;
	bool connect_camera(const std::string &camera_name);
	bool disconnect_camera();
	
	bool configure_camera(const int &width, const int &height, const libcamera::PixelFormat &format);
	
	bool start_camera();
	bool stop_camera();
	
	bool is_connected() const { return has_camera; }
	
	bool get_image(std::vector<uint8_t> frame_buffer);
	
private:
	int queue_request(libcamera::Request *request);
	void process_request(libcamera::Request *request);
	void request_complete(libcamera::Request *request);
	
	bool has_camera;
	bool camera_started;
	
	libcamera::Stream *stream;
	
	std::shared_ptr<libcamera::Camera> camera;
	std::unique_ptr<libcamera::CameraManager> camera_manager;
	
	std::unique_ptr<libcamera::CameraConfiguration> config;
	std::unique_ptr<libcamera::FrameBufferAllocator> allocator;
	std::vector<std::unique_ptr<libcamera::Request>> requests;
	std::map<int, std::pair<void *, unsigned int>> mapped_buffers;
	
	std::queue<libcamera::Request *> request_queue;
	
	libcamera::ControlList controls;
	std::mutex control_mutex;
	std::mutex camera_stop_mutex;
	std::mutex free_requests_mutex;
	
	libcamera::StreamConfiguration stream_config;
};

#endif 

