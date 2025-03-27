#include "LiveView.h"

#include <QApplication>
#include <QWindow>

LiveView::LiveView(QWidget *parent) : QOpenGLWidget(parent), display_width(0), display_height(0), ratio(0.f),
	texture_width(0), texture_height(0), update_texture(false)
{
}

LiveView::~LiveView()
{
}

void LiveView::set_buffer(const int &width, const int &height, std::vector<uint8_t> buffer)
{
	if(width * height != buffer.size()) {
		printf("set_buffer sizes don't match: %i x %i = %lld\n", width, height, buffer.size());
		return;
	}
	
	texture_buffer.resize(width * height, 0);
	memcpy(texture_buffer.data(), buffer.data(), width * height);
	
	texture_width = width;
	texture_height = height;
	
	update_texture = true;
}

void LiveView::initializeGL() 
{
	auto error = glewInit();
	if(error != GLEW_OK) {
		printf("Error initializing glew! (%s)\n", glewGetErrorString(error));
		return;
	}
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	
	live_texture = std::make_shared<Texture>();
	live_texture->create(GL_TEXTURE_2D, GL_R8UI);
	
	program = std::make_shared<Program>("../live2D.vert", "../live2D.frag");
	program->create();
	
	program->bind_parameter("image", live_texture);
}

void LiveView::paintGL() 
{
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	glViewport(0, 0, display_width, display_height);

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	if(texture_buffer.size() == 0) {
		return;
	}
	
	float fw, fh;
	image_dimensions(fw, fh);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(camera_fov, ratio, 0.01, 50.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, 0.0, 1.5, 0.0, 0.0, 1.5 - 2.0, 0.0, 1.0, 0.0);
	
	if(update_texture) {
		live_texture->upload(texture_buffer.data(), texture_width, texture_height, GL_RED_INTEGER);
		update_texture = false;
	}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);

	program->bind();
	
	// Not sure if this is legacy? Displaying a black quad...
	glColor4f(0, 0, 0, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-fw, -fh, 0); // -1 -1
	glTexCoord2f(1.0, 0.0);
	glVertex3f(fw, -fh, 0); // 1 -1
	glTexCoord2f(1.0, 1.0);
	glVertex3f(fw, fh, 0); // 1 1
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-fw, fh, 0); // -1 1
	glEnd();

	program->unbind();
	
	glDisable(GL_DEPTH_TEST);

	// Draw our region of interest on top
	glColor4f(1, 1, 1, 1);

	glBegin(GL_LINES);
	glVertex3f(-fw, -fh, 0);
	glVertex3f(fw, -fh, 0);

	glVertex3f(fw, -fh, 0);
	glVertex3f(fw, fh, 0);

	glVertex3f(fw, fh, 0);
	glVertex3f(-fw, fh, 0);

	glVertex3f(-fw, fh, 0);
	glVertex3f(-fw, -fh, 0);
	glEnd();

	glEnable(GL_DEPTH_TEST);
}

void LiveView::resizeGL(int w, int h) 
{
	auto screen = window()->windowHandle()->screen();

	int mw = w * screen->devicePixelRatio();
	int mh = h * screen->devicePixelRatio();
	
	display_width = mw;
	display_height = mh;
	
	ratio = float(display_width) / float(display_height);
}

void LiveView::mouseMoveEvent(QMouseEvent *p) {}
void LiveView::mousePressEvent(QMouseEvent *p) {}
void LiveView::mouseReleaseEvent(QMouseEvent *p) {}

void LiveView::wheelEvent(QWheelEvent *p){}

void LiveView::image_dimensions(float &fw, float &fh)
{
	float largest = float(std::max(texture_width, texture_height));
	
	fw = texture_width / largest;
	fh = texture_height / largest;
}

