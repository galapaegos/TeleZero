#ifndef _Visualization_Texture_h_
#define _Visualization_Texture_h_

#include <GL/glew.h>

#include "glcheck.h"

class Texture {
  public:
    Texture();

    //! GL_R8UI, GL_R16UI, GL_R32F, etc for internal format.
    void create(GLenum target = GL_TEXTURE_2D, GLenum internal_format = GL_RGBA, const bool &normalized = true);
    void destroy();

    void bind();
    void unbind();

    void upload(unsigned char *ptr, const int64_t &width, const int64_t &height, const GLenum &upload_format);
    void upload(unsigned short *ptr, const int64_t &width, const int64_t &height, const GLenum &upload_format);
    void upload(float *ptr, const int64_t &width, const int64_t &height, const GLenum &upload_format);

    bool updated() const;
    void set_updated(const bool &updated);

    void get_texture_size(int &width, int &height) const;

    unsigned int get_texture_id() const;

  protected:
    GLuint texture_id;

    GLenum texture_target;
    GLenum texture_internal_format;

    GLenum uploaded_format;

    int texture_width;
    int texture_height;

    bool updated_texture;
};

#endif
