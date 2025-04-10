#ifndef _LiveView_h_
#define _LiveView_h_

#include <GL/glew.h>

#include <QMouseEvent>
#include <QOpenGLWidget>

#include "glcheck.h"
#include "Program.h"
#include "Texture.h"

class LiveView : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit LiveView(QWidget *parent = nullptr);
    ~LiveView();
	
	void set_buffer(const int &width, const int &height, const int &channels, std::vector<uint8_t> buffer);
	void set_buffer(const int &width, const int &height, const int &channels, const uint8_t *buffer);

private:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	
	void mouseMoveEvent(QMouseEvent *p);
	void mousePressEvent(QMouseEvent *p);
	void mouseReleaseEvent(QMouseEvent *p);

	void wheelEvent(QWheelEvent *p);
	
	void image_dimensions(float &fw, float &fh);
	
	int display_width;
	int display_height;
	float ratio;
	
	float camera_fov;
	
	bool update_texture;
	int texture_width;
	int texture_height;
	std::vector<uint8_t> texture_buffer;
	
	std::shared_ptr<Program> program;
	std::shared_ptr<Texture> live_texture;
};

#endif
