#include "camera.h"

Camera::Camera() : has_camera(false), stream(nullptr), camera(nullptr), camera_manager(nullptr)
{
}

Camera::~Camera()
{
}

bool Camera::initialize()
{
	camera_manager = std::make_unique<libcamera::CameraManager>();
	camera_manager->start();
	return true;
}

bool Camera::uninitialize()
{
	camera_manager->stop();
	return true;
}

std::vector<std::string> Camera::get_cameras() const
{
	std::vector<std::string> names;
	for(auto const &camera : camera_manager->cameras()) {
		names.push_back(camera->id());
	}
	
	return names;
}

bool Camera::connect_camera(const std::string &camera_name)
{
	camera = camera_manager->get(camera_name);
	
	if(camera == nullptr) {
		printf("Failed to get %s\n", camera_name.c_str());
		return false;
	}
	
	camera->acquire();
	printf("Camera %s acquired\n", camera_name.c_str());
	
	has_camera = true;
	
	return true;
}

bool Camera::disconnect_camera()
{
	if(has_camera) {
		camera->stop();
		
		camera->release();
		camera.reset();
		
		has_camera = false;
	}
}

bool Camera::configure_camera()
{
	if(camera == nullptr) {
		printf("Must connect to camera first!\n");
		return false;
	}
	
	if(has_camera == false) {
		printf("Issue with camera\n");
	}
	
	camera->stop();
	
	config = camera->generateConfiguration({libcamera::StreamRole::StillCapture});

	auto &stream_config = config->at(0);
	std::cout << "Default stillcapture configuration is: " << stream_config.toString() << std::endl;
	
	// Update stream_config here?
	
	config->validate();
	std::cout << "Validated stillcapture configuration is: " << stream_config.toString() << std::endl;
	
	stream = stream_config.stream();
	
	camera->configure(config.get());
	
	return true;
}

bool Camera::get_image(std::vector<uint8_t> frame_buffer)
{
	auto allocator = new libcamera::FrameBufferAllocator(camera);
	for(libcamera::StreamConfiguration &cfg : *config) {
		int ret = allocator->allocate(cfg.stream());
		if(ret < 0) {
			printf("Unable to allocate buffers\n");
			return false;
		}
		size_t allocated = allocator->buffers(cfg.stream()).size();
		printf("Allocated %lld buffers for stream\n", allocated);
	}
	
	libcamera::Stream *stream = stream_config.stream();
	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator->buffers(stream);
	std::vector<std::unique_ptr<libcamera::Request>> requests;
	for(unsigned int i = 0; i < buffers.size(); i++) {
		auto request = camera->createRequest();
		if(!request) {
			printf("Unable to create request\n");
			return false;
		}
		
		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(stream, buffer.get());
		if(ret < 0) {
			printf("Unable set buffer for request\n");
			return false;
		}
		
		requests.push_back(std::move(request));
	}
	
	camera->requestCompleted.connect(process_request);
	
	camera->start();
	for(auto &request : requests) {
		camera->queueRequest(request.get());
	}
	
	return true;
}

void Camera::process_request(libcamera::Request *request)
{
	printf("Request completed %s\n", request->toString().c_str());
}
