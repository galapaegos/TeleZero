#include "Texture.h"

Texture::Texture() :
    texture_id(0), texture_target(GL_TEXTURE_2D), texture_internal_format(GL_RGBA), updated_texture(false),
    uploaded_format(0), texture_width(0), texture_height(0) {}

void Texture::create(GLenum target, GLenum internal_format, const bool &normalized) {
    glGenTextures(1, &texture_id);
    check_error();

    texture_target          = target;
    texture_internal_format = internal_format;

    bind();

    glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    check_error();

    if(normalized) {
        glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    check_error();

    unbind();
    check_error();
}

void Texture::destroy() {
    updated_texture = false;

    texture_width   = 0;
    texture_height  = 0;
    uploaded_format = 0;

    glDeleteTextures(1, &texture_id);
    check_error();
}

void Texture::bind() {
    glBindTexture(texture_target, texture_id);
    check_error();
}

void Texture::unbind() {
    glBindTexture(texture_target, 0);
    check_error();
}

void Texture::upload(unsigned char *ptr, const int64_t &width, const int64_t &height, const GLenum &upload_format) {
    bind();

    if(upload_format == uploaded_format && width == texture_width && height == texture_height) {
        glTexSubImage2D(texture_target, 0, 0, 0, GLsizei(width), GLsizei(height), upload_format, GL_UNSIGNED_BYTE, ptr);
        check_error();
    } else {
        glTexImage2D(texture_target,
                     0,
                     texture_internal_format,
                     GLsizei(width),
                     GLsizei(height),
                     0,
                     upload_format,
                     GL_UNSIGNED_BYTE,
                     ptr);

        check_error();

        texture_width   = width;
        texture_height  = height;
        uploaded_format = upload_format;
    }

    unbind();

    updated_texture = true;
}

bool Texture::updated() const { return updated_texture; }

void Texture::set_updated(const bool &updated) { updated_texture = updated; }

void Texture::get_texture_size(int &width, int &height) const {
    width  = texture_width;
    height = texture_height;
}

unsigned int Texture::get_texture_id() const { return texture_id; }
