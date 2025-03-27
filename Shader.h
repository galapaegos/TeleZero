#ifndef _Visualization_Shader_h_
#define _Visualization_Shader_h_

#include <string>

#include <GL/glew.h>

enum class ShaderType {
    VertexShader   = GL_VERTEX_SHADER,
    FragmentShader = GL_FRAGMENT_SHADER

    // we can add live_view_geometry if its needed.
};

std::string get_program_log(GLuint object);

class Shader {
  public:
    Shader(const GLenum &type);

    void load(const std::string &file);

    bool compile();
    void release();

    void attach(const GLuint &program);
    void detach(const GLuint &program);

  protected:
    GLenum shader_type;
    GLuint shader_id;
    std::string shader_file;
    std::string shader_contents;
    bool shader_compiled;
};

class VertexShader : public Shader {
  public:
    VertexShader();
};

class FragmentShader : public Shader {
  public:
    FragmentShader();
};

#endif // GLSHADER_H
