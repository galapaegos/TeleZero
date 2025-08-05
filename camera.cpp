#include "camera.h"

#include <iomanip>
#include <math.h>
#include <sys/ioctl.h>

#include <linux/dma-buf.h>

#include "tiff.h"

static const std::map<int, std::string> cfa_map =
{
	{ libcamera::properties::draft::ColorFilterArrangementEnum::RGGB, "RGGB" },
	{ libcamera::properties::draft::ColorFilterArrangementEnum::GRBG, "GRBG" },
	{ libcamera::properties::draft::ColorFilterArrangementEnum::GBRG, "GBRG" },
	{ libcamera::properties::draft::ColorFilterArrangementEnum::RGB, "RGB" },
	{ libcamera::properties::draft::ColorFilterArrangementEnum::MONO, "MONO" },
};

static const std::map<libcamera::PixelFormat, unsigned int> bayer_formats =
{
	{ libcamera::formats::SRGGB10_CSI2P, 10 },
	{ libcamera::formats::SGRBG10_CSI2P, 10 },
	{ libcamera::formats::SBGGR10_CSI2P, 10 },
	{ libcamera::formats::R10_CSI2P,     10 },
	{ libcamera::formats::SGBRG10_CSI2P, 10 },
	{ libcamera::formats::SRGGB12_CSI2P, 12 },
	{ libcamera::formats::SGRBG12_CSI2P, 12 },
	{ libcamera::formats::SBGGR12_CSI2P, 12 },
	{ libcamera::formats::SGBRG12_CSI2P, 12 },
	{ libcamera::formats::SRGGB14_CSI2P, 14 },
	{ libcamera::formats::SGRBG14_CSI2P, 14 },
	{ libcamera::formats::SBGGR14_CSI2P, 14 },
	{ libcamera::formats::SGBRG14_CSI2P, 14 },
	{ libcamera::formats::SRGGB16,       16 },
	{ libcamera::formats::SGRBG16,       16 },
	{ libcamera::formats::SBGGR16,       16 },
	{ libcamera::formats::SGBRG16,       16 },
};

Camera::Camera() : has_camera(false), camera_started(false), stream(nullptr), camera(nullptr), camera_manager(nullptr), exposure_time(12000),
	analogue_gain(1), brightness(0.f), contrast(1.f), saturation(1.f), lens_position(0.f), temperature(0.f), lines_per_row(0), padding(0)
{
	supported_formats.push_back(libcamera::formats::XRGB8888);
	//supported_formats.push_back(libcamera::formats::XBGR8888);
	supported_formats.push_back(libcamera::formats::RGBA8888);
	//supported_formats.push_back(libcamera::formats::BGRA8888);
	
	//supported_formats.push_back(libcamera::formats::BGR888);
	supported_formats.push_back(libcamera::formats::RGB888);
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
	
	//10 - [ 20749, 20750, 20737, 20738, 20739 ]
	//8 - (0, 0)/0x0
	//7 - [ (8, 16)/4056x3040 ]
	//5 - 4056x3040
	//2 - 180
	//1 - 2
	//10001 - 0
	//4 - 1550x1550
	//3 - imx477
	const libcamera::ControlList &properties = camera->properties();
	
	auto area = properties.get(libcamera::properties::PixelArraySize).value();
	auto sensor_properties = properties.get(libcamera::properties::Model).value();
	
	//config = camera->generateConfiguration({libcamera::StreamRole::Raw});
	config = camera->generateConfiguration({libcamera::StreamRole::StillCapture});
	//config = camera->generateConfiguration({libcamera::StreamRole::Viewfinder});
	
	auto &stream_config = config->at(0);
	
	if(config->validate() == libcamera::CameraConfiguration::Invalid) {
		printf("Invalid camera configuration\n");
		return false;
	}

	pixel_formats.clear();
	pixel_format_sizes.clear();
	
	//for(const auto &format : formats.pixelformats()) {
	for(auto &format : supported_formats) {
		pixel_formats.push_back(format);
		
		pixel_format_sizes[format].push_back(area);
		pixel_format_sizes[format].push_back(area / 2);
		pixel_format_sizes[format].push_back(area / 4);
		pixel_format_sizes[format].push_back(area / 8);
	}
	
	// Raspberry pi zero 2 w
	// 15 AwbEnable libcamera - [false..true]
	// 18 ColourGains libcamera - [0.000000..32.000000]
	// 16 AwbMode libcamera - [0..7]
	// 19 ColourTemperature libcamera - [100..100000]
	// 20 Saturation libcamera - [0.000000..32.000000]
	// 20007 CnnEnableInputTensor rpi - [false..true]
	// 28 FrameDurationLimits libcamera - [33333..120000]
	// 25 ScalerCrop libcamera - [(0, 0)/0x0..(65535, 65535)/65535x65535]
	// 20001 StatsOutputEnable rpi - [false..true]
	// 10002 NoiseReductionMode draft - [0..4]
	// 22 Sharpness libcamera - [0.000000..16.000000]
	// 12 Brightness libcamera - [-1.000000..1.000000]
	// 4 AeConstraintMode libcamera - [0..3]
	// 20011 SyncMode rpi - [0..2]
	// 5 AeExposureMode libcamera - [0..3]
	// 7 ExposureTime libcamera - [1..66666]
	// 9 AeFlickerMode libcamera - [0..1]
	// 6 ExposureValue libcamera - [-8.000000..8.000000]
	// 20014 SyncFrames rpi - [1..1000000]
	// 1 AeEnable libcamera - [false..true]
	// 8 AnalogueGain libcamera - [1.000000..16.000000]
	// 10 AeFlickerPeriod libcamera - [100..1000000]
	// 13 Contrast libcamera - [0.000000..32.000000]
	// 3 AeMeteringMode libcamera - [0..3]
	// 41 HdrMode libcamera - [0..4]
	
	// Raspberry pi 5
	// 17 AwbEnable - [false..true]
	// 20 ColourGains - [0.000000..32.000000]
	// 18 AwbMode - [0..7]
	// 21 ColourTemperature - [100..100000]
	// 22 Saturation - [0.000000..32.000000]
	// 20003 ScalerCrops - [(0, 0)/0x0..(65535, 65535)/65535x65535]
	// 20007 CnnEnableInputTensor - [false..true]
	// 30 FrameDurationLimits - [33333..120000]
	// 27 ScalerCrop - [(0, 0)/0x0..(65535, 65535)/65535x65535]
	// 10002 NoiseReductionMode - [0..4]
	// 24 Sharpness - [0.000000..16.000000]
	// 1 AeEnable - [false..true]
	// 6 ExposureValue - [-8.000000..8.000000]
	// 20011 SyncMode - [0..2]
	// 11 AeFlickerMode - [0..1]
	// 7 ExposureTime - [1..66666]
	// 20014 SyncFrames - [1..1000000]
	// 14 Brightness - [-1.000000..1.000000]
	// 20001 StatsOutputEnable - [false..true]
	// 9 AnalogueGain - [1.000000..16.000000]
	// 10 AnalogueGainMode - [0..1]
	// 12 AeFlickerPeriod - [100..1000000]
	// 3 AeMeteringMode - [0..3]
	// 43 HdrMode - [0..4]
	// 8 ExposureTimeMode - [0..1]
	// 4 AeConstraintMode - [0..3]
	// 15 Contrast - [0.000000..32.000000]
	// 5 AeExposureMode - [0..3]
	const libcamera::ControlInfoMap &control_info = camera->controls();
	for(auto itr = control_info.begin(); itr != control_info.end(); itr++) {
		auto control_id = itr->first;
		auto control = itr->second;
		printf("%i %s - %s\n", control_id->id(), control_id->name().c_str(), control.toString().c_str());
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
	channels = get_channels(pixel_format);
	printf("Configured to use: %s  %i x %i [%i]\n", pixel_format.toString().c_str(), width, height, channels);
	
	config->at(0).size = pixel_format_size;
	config->at(0).pixelFormat = pixel_format;
	config->at(0).bufferCount = 1;
	
	if(config->validate() == libcamera::CameraConfiguration::Invalid) {
		printf("Invalid camera configuration\n");
		return false;
	}
	
	printf("frameSize: %i [%i] [%i] %i\n", config->at(0).frameSize, width*height*3, width*height*4, config->at(0).stride);
	
	// computing the aligned dimensions:
	int stride = config->at(0).stride;
	lines_per_row = stride / width;
	padding = stride - lines_per_row * width;
	
	printf("offsets %i %i\n", lines_per_row, padding);
	
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
	cam_controls.set(libcamera::controls::AeEnable, false);
	//cam_controls.set(libcamera::controls::AeExposureMode, 2);
	cam_controls.set(libcamera::controls::AwbEnable, false);
	//cam_controls.set(libcamera::controls::AfMode, 0);
	
	cam_controls.set(libcamera::controls::AnalogueGain, analogue_gain);
	cam_controls.set(libcamera::controls::ExposureTime, exposure_time);
	cam_controls.set(libcamera::controls::Brightness, brightness);
	cam_controls.set(libcamera::controls::Contrast, contrast);
	cam_controls.set(libcamera::controls::Saturation, saturation);
	//cam_controls.set(libcamera::controls::Sharpness, sharpness);
	//cam_controls.set(libcamera::controls::LensPosition, 0);
	
	printf("Initial controls:   a:%0.2f  e:%microseconds b:%0.2f c:%0.2f s:%0.2f\n", analogue_gain, exposure_time, brightness, contrast, saturation);
	
	cam_controls.set(libcamera::controls::draft::NoiseReductionMode, libcamera::controls::draft::NoiseReductionModeOff);
	
	printf("camera->start(&cam_controls)\n");	
	auto ret = camera->start(&cam_controls);
	if(ret != 0) {
		printf("Failed to start capturing\n");
		return false;
	}
	
	printf("camera->queueRequest()\n");
	for(std::unique_ptr<libcamera::Request> &request : requests) {
		camera->queueRequest(request.get());
	}
	
	camera_started = true;
	
	printf("Camera started\n");
	
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
		printf("emptying queue\n");
		request_queue.pop();
	}
	
	for(auto &itr : mapped_buffers) {
		auto pair = itr.second;
		munmap(std::get<0>(pair), std::get<1>(pair));
	}
	
	mapped_buffers.clear();
	requests.clear();
	allocator.reset();
	cam_controls.clear();
	
	printf("Camera stopped\n");
	
	return true;
}

bool Camera::get_image(std::vector<uint8_t> &frame_buffer)
{
	//printf("Camera::get_image()\n");
	
	std::vector<uint8_t> aligned_buffer;
	{
		std::lock_guard<std::mutex> lock(free_requests_mutex);
		aligned_buffer.resize(image_buffer.size());
		memcpy(aligned_buffer.data(), image_buffer.data(), image_buffer.size());
	}
	
	frame_buffer.resize(width * height * channels);
	
	int src_idx = 0;
	int dst_idx = 0;
	uint8_t *src = aligned_buffer.data();
	uint8_t *dst = frame_buffer.data();
	while(src_idx < aligned_buffer.size()) {
		for(auto i = 0; i < lines_per_row*width; i++) {
			dst[dst_idx + i] = src[src_idx + i];
		}
		dst_idx += lines_per_row * width;
		src_idx += lines_per_row * width + padding;
	}
	
	//printf("idx: %i %i lpr:%i pad:%i\n", src_idx, dst_idx, lines_per_row, padding);
	
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

int Camera::get_channels(const libcamera::PixelFormat &format) {
	switch(format) {
		case libcamera::formats::XRGB8888:
		case libcamera::formats::XBGR8888:
		case libcamera::formats::RGBA8888:
		case libcamera::formats::BGRA8888:
		{
			return 4;
		}break;
	
		case libcamera::formats::BGR888:
		case libcamera::formats::RGB888:
		{
			return 3;
		}
	}
	
	return -1;
}

int Camera::queue_request(libcamera::Request *request)
{
	std::lock_guard<std::mutex> stop_lock(camera_stop_mutex);
	{
		std::lock_guard<std::mutex> lock(control_mutex);
		request->controls() = std::move(cam_controls);
	}
	
	// printf("camera->queueRequest(request)\n");	
	return camera->queueRequest(request);
}

void Camera::process_request(libcamera::Request *request)
{
	std::lock_guard<std::mutex> lock(free_requests_mutex);
	
	if(request->status() == libcamera::Request::Status::RequestCancelled) {
		printf("status: Cancelled\n");
		return;
	}
	
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
		
		sequence = request->sequence();
		//printf("sequence: %i\n", request->sequence());
		auto &read_controls = request->metadata();
		//for(auto itr = read_controls.begin(); itr != read_controls.end(); itr++) {
		//	printf("control: %i, %s\n", itr->first, itr->second.toString().c_str());
		//}
		//if(read_controls.contains(8)) {
		//	printf("Used controls:   a:%0.2f\n", read_controls.get(8).get<float>());
		//}
		//if(read_controls.contains(7)) {
		//	printf("                 e:%i microseconds\n", read_controls.get(7).get<int>());
		//}
		//if(read_controls.contains(12)) {
		//	printf("                 b:%0.2f\n", read_controls.get(12).get<float>());
		//}
		//if(read_controls.contains(13)) {
		//	printf("                 c:%0.2f\n", read_controls.get(13).get<float>());
		//}
		//if(read_controls.contains(20)) {
		//	printf("                 s:%0.2f\n", read_controls.get(20).get<float>());
		//}
		
		if(read_controls.contains(29)) {
			//temperature = read_controls.get(32).get<float>();
			temperature = read_controls.get(libcamera::controls::SENSOR_TEMPERATURE).get<float>();
		}
	}
	
	request->reuse(libcamera::Request::ReuseBuffers);
	
	// Updating our request values, these are applied in queue_request
	//libcamera::ControlList &controls = request->controls();
	
	cam_controls.set(libcamera::controls::AeEnable, false);
	//cam_controls.set(libcamera::controls::AeExposureMode, 2);
	cam_controls.set(libcamera::controls::AwbEnable, false);
	//cam_controls.set(libcamera::controls::AfMode, 0);
	
	cam_controls.set(libcamera::controls::AnalogueGain, analogue_gain);
	cam_controls.set(libcamera::controls::ExposureTime, exposure_time);
	cam_controls.set(libcamera::controls::Brightness, brightness);
	cam_controls.set(libcamera::controls::Contrast, contrast);
	cam_controls.set(libcamera::controls::Saturation, saturation);
	//cam_controls.set(libcamera::controls::Sharpness, sharpness);
	//cam_controls.set(libcamera::controls::LensPosition, lens_position);
	
	//printf("Updated controls:   a:%0.2f  e:%imicroseconds b:%0.2f c:%0.2f s:%0.2f\n", analogue_gain, exposure_time, brightness, contrast, saturation);
	
	queue_request(request);
	//camera->queueRequest(request);
	
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

