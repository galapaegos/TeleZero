#ifndef _Visualization_Program_h_
#define _Visualization_Program_h_

#include <memory>
#include <string>
#include <vector>

#include "glcheck.h"
#include "Shader.h"
#include "Texture.h"

class Program {
  public:
    Program(const std::string &vertex, const std::string &fragment);

    void create();
    void destroy();

    void bind();
    void unbind();

    void bind_parameter(const std::string &name, std::shared_ptr<Texture> tex);

    void set_uniform(const std::string &loc, const int &value);

  protected:
    inline GLint get_uniform_location(const std::string &loc) { return glGetUniformLocation(program_id, loc.c_str()); }
    inline GLint get_attrib_location(const std::string &loc) { return glGetAttribLocation(program_id, loc.c_str()); }

    std::unique_ptr<FragmentShader> fragment_shader;
    std::unique_ptr<VertexShader> vertex_shader;

    std::string fragment_shader_file;
    std::string vertex_shader_file;

    GLuint program_id;
    bool link_program;

  private:
    struct TextureItem {
        std::string name;
        std::shared_ptr<Texture> value;
    };

    std::vector<TextureItem> textures;
};

#endif
