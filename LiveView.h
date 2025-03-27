#ifndef _LiveView_h_
#define _LiveView_h_

#include <GL/glew.h>

#include <QMouseEvent>
#include <QOpenGLWidget>

#include "Program.h"
#include "Texture.h"

class LiveView : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit LiveView(QWidget *parent = nullptr);
    ~LiveView();
	
	void set_buffer(const int &width, const int &height, std::vector<uint8_t> buffer);

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
	
	void mouseMoveEvent(QMouseEvent *p) override;
	void mousePressEvent(QMouseEvent *p) override;
	void mouseReleaseEvent(QMouseEvent *p) override;

	void wheelEvent(QWheelEvent *p);
	
private:
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
