#ifndef _camera_h_
#define _camera_h_

#undef signals
#undef slots
#undef emit
#undef foreach

#include <libcamera/libcamera.h>

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
	
	bool configure_camera();
	
	bool is_connect() const { return has_camera; }
	
	bool get_image(std::vector<uint8_t> frame_buffer);
	
private:
	void process_request(libcamera::Request *request);
	bool has_camera;
	
	libcamera::Stream *stream;
	
	std::shared_ptr<libcamera::Camera> camera;
	std::unique_ptr<libcamera::CameraManager> camera_manager;
	
	std::unique_ptr<libcamera::CameraConfiguration> config;
	libcamera::StreamConfiguration stream_config;
};

#endif 

