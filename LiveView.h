#ifndef _LiveView_h_
#define _LiveView_h_

#include <GL/glew.h>

#include <QWidget>
#include <QMouseEvent>
#include <QOpenGLWidget>

#include "glcheck.h"
#include "Program.h"
#include "Texture.h"
#include "trackball.h"

class LiveView : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit LiveView(QWidget *parent = nullptr);
    ~LiveView();
	
	void set_crosshair_visible(const bool &visible) { show_crosshair = visible;}

	void set_texture_type(const GLenum &format);
	
	void set_buffer(const int &width, const int &height, const int &channels, std::vector<uint8_t> buffer);
	void set_buffer(const int &width, const int &height, const int &channels, const uint8_t *buffer);
	
	int color_order;

private:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	
	void mouseMoveEvent(QMouseEvent *p);
	void mousePressEvent(QMouseEvent *p);
	void mouseReleaseEvent(QMouseEvent *p);

	void wheelEvent(QWheelEvent *p);
	
	void image_dimensions(float &fw, float &fh);

	glm::vec3 translate;
	
	int display_width;
	int display_height;
	float ratio;
	
	float camera_fov;

	bool show_crosshair;
	
	bool update_texture;
	int texture_width;
	int texture_height;
	int texture_channel;
	std::vector<uint8_t> texture_buffer;
	
	std::shared_ptr<Program> program;
	std::shared_ptr<Texture> live_texture;

	Trackball trackball;
};

#endif
