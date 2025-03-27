#include "camera.h"

Camera::Camera() : has_camera(false), camera_started(false), stream(nullptr), camera(nullptr), camera_manager(nullptr)
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

bool Camera::configure_camera(const int &width, const int &height, const libcamera::PixelFormat &format)
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

	config = camera->generateConfiguration({libcamera::StreamRole::StillCapture});

	auto &stream_config = config->at(0);
	
	if(width && height) {
		libcamera::Size size(width, height);
		config->at(0).size = size;
	}
	
	config->at(0).pixelFormat = format;
	config->at(0).bufferCount = 1;
	
	if(config->validate() == libcamera::CameraConfiguration::Invalid) {
		printf("Invalid camera configuration\n");
		return false;
	}
	
	return true;
}

bool Camera::start_camera()
{
	auto ret = camera->configure(config.get());
	if(ret < 0) {
		printf("Failed to configure camera\n");
		return false;
	}
	
	camera->requestCompleted.connect(this, &Camera::request_complete);
	
	allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
	for(libcamera::StreamConfiguration &cfg : *config) {
		int ret = allocator->allocate(cfg.stream());
		if(ret < 0) {
			printf("Unable to allocate buffers\n");
			return false;
		}
		size_t allocated = allocator->buffers(cfg.stream()).size();
		printf("Allocated %lld buffers for stream\n", allocated);
	}
	
	auto request = camera->createRequest();
	if(!request) {
		printf("Can't create request\n");
		return false;
	}
	
	for(libcamera::StreamConfiguration &cfg : *config) {
		libcamera::Stream *stream = cfg.stream();
		
		const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator->buffers(stream);
		if(buffers.size() < 1) {
			return false;
		}
		const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[0];
		
		auto ret = request->addBuffer(stream, buffer.get());
		if(ret < 0) {
			printf("Can't set buffer for request\n");
			return false;
		}
		
		for(const libcamera::FrameBuffer::Plane &plane : buffer->planes()) {
			void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
			mapped_buffers[plane.fd.get()] = std::make_pair(memory, plane.length);
		}
	}
	
	requests.push_back(std::move(request));
	
	ret = camera->start(&controls);
	if(ret != 0) {
		printf("Failed to start capturing\n");
		return false;
	}
	
	camera_started = true;
	
	controls.clear();
	for(auto &request : requests) {
		auto ret = queue_request(request.get());
		if(ret < 0) {
			printf("Can't queue request\n");
			camera->stop();
			return false;
		}
	}
	
	stream = config->at(0).stream();
	
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

bool Camera::get_image(std::vector<uint8_t> frame_buffer)
{
	std::lock_guard<std::mutex> lock(free_requests_mutex);
	if(!requests.empty()) {
		auto request = request_queue.front();
		
		const libcamera::Request::BufferMap &buffers = request->buffers();
		for(auto itr = buffers.begin(); itr != buffers.end(); itr++) {
			auto buffer = itr->second;
			
			for(unsigned int i = 0; i < buffer->planes().size(); i++) {
				const libcamera::FrameBuffer::Plane &plane = buffer->planes()[i];
				const libcamera::FrameMetadata::Plane &meta = buffer->metadata().planes()[i];
				
				void *data = mapped_buffers[plane.fd.get()].first;
				auto length = std::min(meta.bytesused, plane.length);
				
				frame_buffer.resize(length);
				memcpy(frame_buffer.data(), data, length);
			}
		}
		
		request_queue.pop();
		return true;
	}
	
	return false;
}

int Camera::queue_request(libcamera::Request *request)
{
	std::lock_guard<std::mutex> stop_lock(camera_stop_mutex);
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		request->controls() = std::move(controls);
	}
	
	return camera->queueRequest(request);
}

void Camera::process_request(libcamera::Request *request)
{
	request_queue.push(request);
	printf("Request completed %s\n", request->toString().c_str());
}

void Camera::request_complete(libcamera::Request *request)
{
	if(request->status() == libcamera::Request::RequestCancelled) {
		return;
	}
	
	process_request(request);
	
	printf("Request completed %s\n", request->toString().c_str());
}


std::string convert_format(const libcamera::PixelFormat &format){
	return "YUV420";
}

libcamera::PixelFormat convert_format(const std::string &format){
	return libcamera::formats::YUV420;
}
	/*
 constexpr PixelFormat R8{ __fourcc('R', '8', ' ', ' '), __mod(0, 0) };
 constexpr PixelFormat R10{ __fourcc('R', '1', '0', ' '), __mod(0, 0) };
 constexpr PixelFormat R12{ __fourcc('R', '1', '2', ' '), __mod(0, 0) };
 constexpr PixelFormat R16{ __fourcc('R', '1', '6', ' '), __mod(0, 0) };
 constexpr PixelFormat RGB565{ __fourcc('R', 'G', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat RGB565_BE{ __fourcc('R', 'G', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat RGB888{ __fourcc('R', 'G', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat BGR888{ __fourcc('B', 'G', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat XRGB8888{ __fourcc('X', 'R', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat XBGR8888{ __fourcc('X', 'B', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat RGBX8888{ __fourcc('R', 'X', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat BGRX8888{ __fourcc('B', 'X', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat ARGB8888{ __fourcc('A', 'R', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat ABGR8888{ __fourcc('A', 'B', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat RGBA8888{ __fourcc('R', 'A', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat BGRA8888{ __fourcc('B', 'A', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat RGB161616{ __fourcc('R', 'G', '4', '8'), __mod(0, 0) };
 constexpr PixelFormat BGR161616{ __fourcc('B', 'G', '4', '8'), __mod(0, 0) };
 constexpr PixelFormat YUYV{ __fourcc('Y', 'U', 'Y', 'V'), __mod(0, 0) };
 constexpr PixelFormat YVYU{ __fourcc('Y', 'V', 'Y', 'U'), __mod(0, 0) };
 constexpr PixelFormat UYVY{ __fourcc('U', 'Y', 'V', 'Y'), __mod(0, 0) };
 constexpr PixelFormat VYUY{ __fourcc('V', 'Y', 'U', 'Y'), __mod(0, 0) };
 constexpr PixelFormat AVUY8888{ __fourcc('A', 'V', 'U', 'Y'), __mod(0, 0) };
 constexpr PixelFormat XVUY8888{ __fourcc('X', 'V', 'U', 'Y'), __mod(0, 0) };
 constexpr PixelFormat NV12{ __fourcc('N', 'V', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat NV21{ __fourcc('N', 'V', '2', '1'), __mod(0, 0) };
 constexpr PixelFormat NV16{ __fourcc('N', 'V', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat NV61{ __fourcc('N', 'V', '6', '1'), __mod(0, 0) };
 constexpr PixelFormat NV24{ __fourcc('N', 'V', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat NV42{ __fourcc('N', 'V', '4', '2'), __mod(0, 0) };
 constexpr PixelFormat YUV420{ __fourcc('Y', 'U', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat YVU420{ __fourcc('Y', 'V', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat YUV422{ __fourcc('Y', 'U', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat YVU422{ __fourcc('Y', 'V', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat YUV444{ __fourcc('Y', 'U', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat YVU444{ __fourcc('Y', 'V', '2', '4'), __mod(0, 0) };
 constexpr PixelFormat MJPEG{ __fourcc('M', 'J', 'P', 'G'), __mod(0, 0) };
 constexpr PixelFormat SRGGB8{ __fourcc('R', 'G', 'G', 'B'), __mod(0, 0) };
 constexpr PixelFormat SGRBG8{ __fourcc('G', 'R', 'B', 'G'), __mod(0, 0) };
 constexpr PixelFormat SGBRG8{ __fourcc('G', 'B', 'R', 'G'), __mod(0, 0) };
 constexpr PixelFormat SBGGR8{ __fourcc('B', 'A', '8', '1'), __mod(0, 0) };
 constexpr PixelFormat SRGGB10{ __fourcc('R', 'G', '1', '0'), __mod(0, 0) };
 constexpr PixelFormat SGRBG10{ __fourcc('B', 'A', '1', '0'), __mod(0, 0) };
 constexpr PixelFormat SGBRG10{ __fourcc('G', 'B', '1', '0'), __mod(0, 0) };
 constexpr PixelFormat SBGGR10{ __fourcc('B', 'G', '1', '0'), __mod(0, 0) };
 constexpr PixelFormat SRGGB12{ __fourcc('R', 'G', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat SGRBG12{ __fourcc('B', 'A', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat SGBRG12{ __fourcc('G', 'B', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat SBGGR12{ __fourcc('B', 'G', '1', '2'), __mod(0, 0) };
 constexpr PixelFormat SRGGB14{ __fourcc('R', 'G', '1', '4'), __mod(0, 0) };
 constexpr PixelFormat SGRBG14{ __fourcc('B', 'A', '1', '4'), __mod(0, 0) };
 constexpr PixelFormat SGBRG14{ __fourcc('G', 'B', '1', '4'), __mod(0, 0) };
 constexpr PixelFormat SBGGR14{ __fourcc('B', 'G', '1', '4'), __mod(0, 0) };
 constexpr PixelFormat SRGGB16{ __fourcc('R', 'G', 'B', '6'), __mod(0, 0) };
 constexpr PixelFormat SGRBG16{ __fourcc('G', 'R', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat SGBRG16{ __fourcc('G', 'B', '1', '6'), __mod(0, 0) };
 constexpr PixelFormat SBGGR16{ __fourcc('B', 'Y', 'R', '2'), __mod(0, 0) };
 constexpr PixelFormat R10_CSI2P{ __fourcc('R', '1', '0', ' '), __mod(11, 1) };
 constexpr PixelFormat R12_CSI2P{ __fourcc('R', '1', '2', ' '), __mod(11, 1) };
 constexpr PixelFormat SRGGB10_CSI2P{ __fourcc('R', 'G', '1', '0'), __mod(11, 1) };
 constexpr PixelFormat SGRBG10_CSI2P{ __fourcc('B', 'A', '1', '0'), __mod(11, 1) };
 constexpr PixelFormat SGBRG10_CSI2P{ __fourcc('G', 'B', '1', '0'), __mod(11, 1) };
 constexpr PixelFormat SBGGR10_CSI2P{ __fourcc('B', 'G', '1', '0'), __mod(11, 1) };
 constexpr PixelFormat SRGGB12_CSI2P{ __fourcc('R', 'G', '1', '2'), __mod(11, 1) };
 constexpr PixelFormat SGRBG12_CSI2P{ __fourcc('B', 'A', '1', '2'), __mod(11, 1) };
 constexpr PixelFormat SGBRG12_CSI2P{ __fourcc('G', 'B', '1', '2'), __mod(11, 1) };
 constexpr PixelFormat SBGGR12_CSI2P{ __fourcc('B', 'G', '1', '2'), __mod(11, 1) };
 constexpr PixelFormat SRGGB14_CSI2P{ __fourcc('R', 'G', '1', '4'), __mod(11, 1) };
 constexpr PixelFormat SGRBG14_CSI2P{ __fourcc('B', 'A', '1', '4'), __mod(11, 1) };
 constexpr PixelFormat SGBRG14_CSI2P{ __fourcc('G', 'B', '1', '4'), __mod(11, 1) };
 constexpr PixelFormat SBGGR14_CSI2P{ __fourcc('B', 'G', '1', '4'), __mod(11, 1) };
 constexpr PixelFormat SRGGB10_IPU3{ __fourcc('R', 'G', '1', '0'), __mod(1, 13) };
 constexpr PixelFormat SGRBG10_IPU3{ __fourcc('B', 'A', '1', '0'), __mod(1, 13) };
 constexpr PixelFormat SGBRG10_IPU3{ __fourcc('G', 'B', '1', '0'), __mod(1, 13) };
 constexpr PixelFormat SBGGR10_IPU3{ __fourcc('B', 'G', '1', '0'), __mod(1, 13) };
 constexpr PixelFormat RGGB_PISP_COMP1{ __fourcc('R', 'G', 'B', '6'), __mod(12, 1) };
 constexpr PixelFormat GRBG_PISP_COMP1{ __fourcc('G', 'R', '1', '6'), __mod(12, 1) };
 constexpr PixelFormat GBRG_PISP_COMP1{ __fourcc('G', 'B', '1', '6'), __mod(12, 1) };
 constexpr PixelFormat BGGR_PISP_COMP1{ __fourcc('B', 'Y', 'R', '2'), __mod(12, 1) };
 constexpr PixelFormat MONO_PISP_COMP1{ __fourcc('R', '1', '6', ' '), __mod(12, 1) };
 */
 
 /*
  enum {
     AE_ENABLE = 1,
     AE_STATE = 2,
     AE_METERING_MODE = 3,
     AE_CONSTRAINT_MODE = 4,
     AE_EXPOSURE_MODE = 5,
     EXPOSURE_VALUE = 6,
     EXPOSURE_TIME = 7,
     EXPOSURE_TIME_MODE = 8,
     ANALOGUE_GAIN = 9,
     ANALOGUE_GAIN_MODE = 10,
     AE_FLICKER_MODE = 11,
     AE_FLICKER_PERIOD = 12,
     AE_FLICKER_DETECTED = 13,
     BRIGHTNESS = 14,
     CONTRAST = 15,
     LUX = 16,
     AWB_ENABLE = 17,
     AWB_MODE = 18,
     AWB_LOCKED = 19,
     COLOUR_GAINS = 20,
     COLOUR_TEMPERATURE = 21,
     SATURATION = 22,
     SENSOR_BLACK_LEVELS = 23,
     SHARPNESS = 24,
     FOCUS_FO_M = 25,
     COLOUR_CORRECTION_MATRIX = 26,
     SCALER_CROP = 27,
     DIGITAL_GAIN = 28,
     FRAME_DURATION = 29,
     FRAME_DURATION_LIMITS = 30,
     SENSOR_TEMPERATURE = 31,
     SENSOR_TIMESTAMP = 32,
     AF_MODE = 33,
     AF_RANGE = 34,
     AF_SPEED = 35,
     AF_METERING = 36,
     AF_WINDOWS = 37,
     AF_TRIGGER = 38,
     AF_PAUSE = 39,
     LENS_POSITION = 40,
     AF_STATE = 41,
     AF_PAUSE_STATE = 42,
     HDR_MODE = 43,
     HDR_CHANNEL = 44,
     GAMMA = 45,
     DEBUG_METADATA_ENABLE = 46,
 };
 */