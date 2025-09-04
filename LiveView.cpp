#include "LiveView.h"

#include <QApplication>
#include <QScreen>
#include <QWindow>

LiveView::LiveView(QWidget *parent) :
    QOpenGLWidget(parent), display_width(0), display_height(0), ratio(0.f), camera_fov(55.f), texture_width(0),
    texture_height(0), update_texture(false), translate(0.f, 0.f, 1.5f), show_crosshair(false) {}

LiveView::~LiveView() {}

void LiveView::set_texture_type(const GLenum &format) {
    live_texture->destroy();

    live_texture->create(GL_TEXTURE_2D, format, false);
}

void LiveView::set_buffer(const int &width, const int &height, const int &channels, std::vector<uint8_t> buffer) {
    if(width * height * channels != buffer.size()) {
        printf("set_buffer sizes don't match: %i x %i = %lld\n", width, height, buffer.size());
        return;
    }

    texture_buffer.resize(width * height * channels, 0);
    memcpy(texture_buffer.data(), buffer.data(), width * height * channels);

    texture_width   = width;
    texture_height  = height;
    texture_channel = channels;

    update_texture = true;
}

void LiveView::set_buffer(const int &width, const int &height, const int &channels, const uint8_t *buffer) {
    texture_buffer.resize(width * height, 0);
    memcpy(texture_buffer.data(), buffer, width * height);

    texture_width   = width;
    texture_height  = height;
    texture_channel = channels;

    update_texture = true;
}

void LiveView::initializeGL() {
    auto error = glewInit();
    if(error != GLEW_OK) {
        printf("Error initializing glew! (%s)\n", glewGetErrorString(error));
        return;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    check_error();

    live_texture = std::make_shared<Texture>();
    live_texture->create(GL_TEXTURE_2D, GL_RGBA8UI, false);
    check_error();

    program = std::make_shared<Program>("../live2D.vert", "../live2D.frag");
    program->create();
    check_error();

    program->bind_parameter("image", live_texture);
    check_error();
}

void LiveView::paintGL() {
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    check_error();

    glViewport(0, 0, display_width, display_height);
    check_error();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    check_error();

    if(texture_buffer.size() == 0) {
        return;
    }

    float fw, fh;
    image_dimensions(fw, fh);
    // printf("image_dimensions: %f,%f\n", fw, fh);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera_fov, ratio, 0.1, 5.0);
    check_error();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // gluLookAt(0.0, 0.0, 2.25, 0.0, 0.0, 2.25 - 2.0, 0.0, 1.0, 0.0);
    gluLookAt(translate.x, -translate.y, translate.z, translate.x, -translate.y, translate.z - 2.0, 0, 1, 0);
    check_error();

    if(update_texture) {
        switch(texture_channel) {
            case 3: {
                live_texture->upload(texture_buffer.data(), texture_width, texture_height, GL_RGB_INTEGER);
            } break;

            case 4: {
                live_texture->upload(texture_buffer.data(), texture_width, texture_height, GL_RGBA_INTEGER);
            }
        }
        update_texture = false;
        check_error();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    check_error();

    program->bind();
    check_error();

    program->set_uniform("channels", texture_channel);
    program->set_uniform("color_order", color_order);
    check_error();

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
    check_error();

    program->unbind();
    check_error();

    glDisable(GL_DEPTH_TEST);
    check_error();

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
    check_error();

    if(show_crosshair) {
        glColor4f(1, 0, 0, 1);
        glBegin(GL_LINES);
        glVertex3f(0, -fh, 0);
        glVertex3f(0, fh, 0);

        glVertex3f(-fw, 0, 0);
        glVertex3f(fw, 0, 0);
        glEnd();
    }

    glEnable(GL_DEPTH_TEST);
    check_error();
}

void LiveView::resizeGL(int w, int h) {
    // printf("resize: %i, %i\n", w, h);
    auto screen = window()->windowHandle()->screen();

    int mw = w * screen->devicePixelRatio();
    int mh = h * screen->devicePixelRatio();

    display_width  = mw;
    display_height = mh;

    ratio = float(display_width) / float(display_height);
}

void LiveView::mouseMoveEvent(QMouseEvent *p) {
    auto screen = window()->windowHandle()->screen();

    auto window = size();

    int sw = window.width() * screen->devicePixelRatio();
    int sh = window.height() * screen->devicePixelRatio();

    int tx = p->position().x() * screen->devicePixelRatio();
    int ty = p->position().y() * screen->devicePixelRatio();

    int x = tx;
    int y = display_height - ty;

    if(p->buttons() == Qt::LeftButton) {
    } else if(p->buttons() == Qt::MiddleButton) {
        trackball.update(x, y);
    } else if(p->buttons() == Qt::RightButton) {
    }
}

void LiveView::mousePressEvent(QMouseEvent *p) {
    auto screen = window()->windowHandle()->screen();

    auto window = size();

    int sw = window.width() * screen->devicePixelRatio();
    int sh = window.height() * screen->devicePixelRatio();

    int tx = p->position().x() * screen->devicePixelRatio();
    int ty = p->position().y() * screen->devicePixelRatio();

    int x = tx;
    int y = display_height - ty;

    // left mouse button
    if(p->buttons() == Qt::LeftButton) {
    } else if(p->buttons() == Qt::MiddleButton) {
        trackball.setManipulation(Manipulation::Manipulation_TransX, Manipulation::Manipulation_TransY);
        trackball.grab(x, y);
    } else if(p->buttons() == Qt::RightButton) {
    }
}

void LiveView::mouseReleaseEvent(QMouseEvent *p) {
    auto screen = window()->windowHandle()->screen();

    auto window = size();

    int sw = window.width() * screen->devicePixelRatio();
    int sh = window.height() * screen->devicePixelRatio();

    int tx = p->position().x() * screen->devicePixelRatio();
    int ty = p->position().y() * screen->devicePixelRatio();

    int x = tx;
    int y = display_height - ty;

    if(p->button() == Qt::LeftButton) {
    } else if(p->button() == Qt::MiddleButton) {
        trackball.release();

        auto t = trackball.getTranslate();
        translate.x -= t[0];
        translate.y += t[1];

        trackball.identity();
    } else if(p->button() == Qt::RightButton) {
    }
}

void LiveView::wheelEvent(QWheelEvent *p) {
    // Increment the z distance by value
    glm::ivec2 delta(p->angleDelta().x() / 15, p->angleDelta().y() / 15);

    float factor = 0.01f;
    if(translate.z - delta.y * factor > 0.25f && translate.z - delta.y * factor <= 2.f) {
        translate.z -= delta.y * factor;

        trackball.setMultiplier(Manipulation::Manipulation_TransX, 0.003f * (translate.z / 2.f));
        trackball.setMultiplier(Manipulation::Manipulation_TransY, 0.003f * (translate.z / 2.f));
    }
}

void LiveView::image_dimensions(float &fw, float &fh) {
    float largest = float(std::max(texture_width, texture_height));

    fw = texture_width / largest;
    fh = texture_height / largest;
}
