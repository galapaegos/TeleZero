#include "camera.h"

#include <sys/ioctl.h>

#include <linux/dma-buf.h>

#include "tiff.h"

Camera::Camera() : has_camera(false), camera_started(false), stream(nullptr), camera(nullptr), camera_manager(nullptr), exposure_time(12000),
	analogue_gain(0), brightness(0.f), contrast(1.f), saturation(1.f), sharpness(0.f), lens_position(0.f), temperature(0.f)
{
}

Camera::~Camera()
{
}

bool Camera::initialize()
{
	camera_manager = std::make_unique<libcamera::CameraManager>();
	auto ret = camera_manager->start();
	if(ret != 0) {
		printf("Failed to start camera manager (%i)\n", ret);
		return false;
	}
	
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
	
	auto ret = camera->acquire();
	if(ret != 0) {
		printf("Failed to acquire camera (%i)\n", ret);
		return false;
	}
	printf("Camera %s acquired\n", camera_name.c_str());
	
	has_camera = true;
	
	config = camera->generateConfiguration({libcamera::StreamRole::StillCapture});
	//config = camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
	
	auto const &formats = config->at(0).formats();
	for(const auto &format : formats.pixelformats()) {
		pixel_formats.push_back(format);
		//printf("format: %s\n", format.toString().c_str());
		
		pixel_format_sizes[format] = formats.sizes(format);
	}
	
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
	
	return true;
}

//user@telezero:~/TeleZero/build $ ./TeleZero
//[1:21:54.134006738] [2442]  INFO Camera camera_manager.cpp:327 libcamera v0.4.0+53-29156679
//[1:21:54.277936882] [2448]  WARN RPiSdn sdn.cpp:40 Using legacy SDN tuning - please consider moving SDN inside rpi.denoise
//[1:21:54.283708309] [2448]  INFO RPI vc4.cpp:447 Registered camera /base/soc/i2c0mux/i2c@1/imx477@1a to Unicam device /dev/media3 and ISP device /dev/media0
//name: /base/soc/i2c0mux/i2c@1/imx477@1a
//Camera /base/soc/i2c0mux/i2c@1/imx477@1a acquired
//Default stillcapture configuration is: 4056x3040-YUV420
//Validated stillcapture configuration is: 4056x3040-YUV420
//[1:22:01.797244530] [2442]  INFO Camera camera.cpp:1202 configuring streams: (0) 4056x3040-YUV420
//[1:22:01.798293057] [2448]  INFO RPI vc4.cpp:622 Sensor: /base/soc/i2c0mux/i2c@1/imx477@1a - Selected sensor format: 4056x3040-SBGGR12_1X12 - Selected unicam format: 4056x3040-pBCC

bool Camera::configure_camera(const int &format_index, const int &size_index)
{
	printf("Configuring camera for still captures\n");
	
	if(camera == nullptr) {
		printf("Must connect to camera first!\n");
		return false;
	}
	
	if(has_camera == false) {
		printf("Issue with camera\n");
	}
	
	//[125:45:38.952592274] [13485]  INFO Camera camera_manager.cpp:327 libcamera v0.4.0+53-29156679
	//[125:45:39.931733241] [13488]  WARN RPiSdn sdn.cpp:40 Using legacy SDN tuning - please consider moving SDN inside rpi.denoise
	//[125:45:39.937583623] [13488]  INFO RPI vc4.cpp:447 Registered camera /base/soc/i2c0mux/i2c@1/imx477@1a to Unicam device /dev/media3 and ISP device /dev/media0
	//[125:45:39.937748777] [13488]  INFO RPI pipeline_base.cpp:1121 Using configuration file '/usr/share/libcamera/pipeline/rpi/vc4/rpi_apps.yaml'
	//Made QT preview window
	//Mode selection for 2028:1520:12:P
	//    SRGGB10_CSI2P,1332x990/0 - Score: 3456.22
	//    SRGGB12_CSI2P,2028x1080/0 - Score: 1083.84
	//    SRGGB12_CSI2P,2028x1520/0 - Score: 0
	//    SRGGB12_CSI2P,4056x3040/0 - Score: 887
	//[125:45:45.447248957] [13485]  INFO Camera camera.cpp:1202 configuring streams: (0) 2028x1520-YUV420 (1) 2028x1520-SBGGR12_CSI2P
	//[125:45:45.448189308] [13488]  INFO RPI vc4.cpp:622 Sensor: /base/soc/i2c0mux/i2c@1/imx477@1a - Selected sensor format: 2028x1520-SBGGR12_1X12 - Selected unicam format: 2028x1520-pBCC

	auto &stream_config = config->at(0);
	
	auto pixel_format = pixel_formats[format_index];
	auto pixel_format_size = pixel_format_sizes[pixel_format][size_index];
	
	width = pixel_format_size.width;
	height = pixel_format_size.height;
	printf("Configured to use: %i x %i\n", width, height);
	
	config->at(0).size = pixel_format_size;
	config->at(0).pixelFormat = pixel_format;
	config->at(0).bufferCount = 1;
	
	if(config->validate() == libcamera::CameraConfiguration::Invalid) {
		printf("Invalid camera configuration\n");
		return false;
	}
	
	printf("configure camera\n");
	auto ret = camera->configure(config.get());
	if(ret < 0) {
		printf("Failed to configure camera\n");
		return false;
	}
	
	printf("create allocator\n");
	allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
	for(libcamera::StreamConfiguration &cfg : *config) {
		printf("allocator->allocate(cfg.stream)\n");		
		int ret = allocator->allocate(cfg.stream());
		if(ret < 0) {
			printf("Unable to allocate buffers\n");
			return false;
		}
		size_t allocated = allocator->buffers(cfg.stream()).size();
		printf("Allocated %lld buffers for stream\n", allocated);
	}
	
	printf("createRequest\n");		
	auto request = camera->createRequest();
	if(!request) {
		printf("Can't create request\n");
		return false;
	}
	
	printf("requests.push_back(std::move(request))\n");	
	requests.push_back(std::move(request));
	
	libcamera::Stream *stream = stream_config.stream();
		
	printf("allocator->buffers(stream)\n");		
	const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator->buffers(stream);
	
	for(unsigned int i = 0; i < buffers.size(); i++) {
		auto request = camera->createRequest();
		if(!request) {
			printf("Can't create request\n");
			return false;
		}
		
		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
		int ret = request->addBuffer(stream, buffer.get());
		if(ret < 0) {
			printf("Can't set buffer for request\n");
			return false;
		}
		
		requests.push_back(std::move(request));
	}
	
	return true;
}

bool Camera::start_camera()
{
	printf("camera->requestCompleted.connect()\n");
	camera->requestCompleted.connect(this, &Camera::request_complete);
	
	// Initial startup values (https://github.com/libcamera-org/libcamera/blob/master/src/libcamera/control_ids_core.yaml)
	controls.set(libcamera::controls::AwbEnable, false);
	controls.set(libcamera::controls::AeEnable, false);
	controls.set(libcamera::controls::AfMode, 0);
	
	controls.set(libcamera::controls::AnalogueGain, analogue_gain);
	controls.set(libcamera::controls::ExposureTime, exposure_time);
	controls.set(libcamera::controls::Brightness, brightness);
	controls.set(libcamera::controls::Contrast, contrast);
	controls.set(libcamera::controls::Saturation, saturation);
	controls.set(libcamera::controls::Sharpness, sharpness);
	//controls.set(libcamera::controls::LensPosition, lens_position);
	
	printf("camera->start(&controls)\n");	
	auto ret = camera->start(&controls);
	if(ret != 0) {
		printf("Failed to start capturing\n");
		return false;
	}
	
	printf("camera->queueRequest()\n");
	for(std::unique_ptr<libcamera::Request> &request : requests) {
		camera->queueRequest(request.get());
	}
	
	camera_started = true;
	
	return true;
}

bool Camera::stop_camera()
{
	if(camera) {
		{
			std::lock_guard<std::mutex> lock(camera_stop_mutex);
			if(camera_started) {
				if(camera->stop()) {
					printf("Failed to stop camera\n");
					return false;
				}
				
				camera_started = false;
			}
		}
		camera->requestCompleted.disconnect(this, &Camera::request_complete);
	}
	while(!request_queue.empty()) {
		request_queue.pop();
	}
	
	for(auto &itr : mapped_buffers) {
		auto pair = itr.second;
		munmap(std::get<0>(pair), std::get<1>(pair));
	}
	
	mapped_buffers.clear();
	requests.clear();
	allocator.reset();
	controls.clear();
	
	return true;
}

bool Camera::get_image(int &w, int &h, std::vector<uint8_t> &frame_buffer)
{
	//printf("Camera::get_image()\n");
	
	std::lock_guard<std::mutex> lock(free_requests_mutex);
	frame_buffer.resize(image_buffer.size());
	memcpy(frame_buffer.data(), image_buffer.data(), image_buffer.size());
	
	w = width;
	h = height;
	
	return true;
}

std::vector<std::string> Camera::get_pixel_formats() const { 
	std::vector<std::string> formats;
	for(const auto &format : pixel_formats) {
		formats.push_back(format.toString());
	}

	return formats; 
}

std::vector<std::string> Camera::get_pixel_format_sizes(const std::string &format) {
	auto pixel_format = libcamera::PixelFormat::fromString(format);
	if(pixel_format_sizes.find(pixel_format) == pixel_format_sizes.end()) {
		return {};
	}
	
	auto size_list = pixel_format_sizes[pixel_format];
	
	std::vector<std::string> sizes;
	for(const auto &size : size_list) {
		sizes.push_back(size.toString());
	}
	
	return sizes;
}

int Camera::queue_request(libcamera::Request *request)
{
	std::lock_guard<std::mutex> stop_lock(camera_stop_mutex);
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		request->controls() = std::move(controls);
	}
	
	printf("camera->queueRequest(request)\n");	
	return camera->queueRequest(request);
}

void Camera::process_request(libcamera::Request *request)
{
	std::lock_guard<std::mutex> lock(free_requests_mutex);
	
	const libcamera::Request::BufferMap &buffers = request->buffers();
	for (auto bufferPair : buffers) {
		// (Unused) Stream *stream = bufferPair.first;
		libcamera::FrameBuffer *buffer = bufferPair.second;
		const libcamera::FrameMetadata &metadata = buffer->metadata();
		
		auto plane = buffer->planes()[0];
		
		//printf("nplane: %i\n", nplane);
		auto *addr = mmap(nullptr, plane.length, PROT_READ, MAP_PRIVATE, plane.fd.get(), plane.offset);
		if(addr == MAP_FAILED) {
			printf("Unable to map plane %i size %i\n", plane.fd.get(), plane.length);
		}
		
		image_buffer.resize(plane.length);
		memcpy(image_buffer.data(), addr, plane.length);
		if(munmap(addr, plane.length) != 0) {
			printf("Unable to unmap plane\n");
		}
	}
	
	request->reuse(libcamera::Request::ReuseBuffers);
	
	// Updating our request values:
	libcamera::ControlList &controls = request->controls();
	controls.set(libcamera::controls::AnalogueGain, analogue_gain);
	controls.set(libcamera::controls::ExposureTime, exposure_time);
	controls.set(libcamera::controls::Brightness, brightness);
	controls.set(libcamera::controls::Contrast, contrast);
	controls.set(libcamera::controls::Saturation, saturation);
	controls.set(libcamera::controls::Sharpness, sharpness);
	//controls.set(libcamera::controls::LensPosition, lens_position);
	
	//temperature = controls.get(libcamera::controls::SensorTemperature).value();
	
	camera->queueRequest(request);
	
	//printf("Request completed %s\n", request->toString().c_str());
}

void Camera::request_complete(libcamera::Request *request)
{
	if(request->status() == libcamera::Request::RequestCancelled) {
		return;
	}
	
	//printf("Request completed %s\n", request->toString().c_str());
	process_request(request);
}

