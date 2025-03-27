#include "Shader.h"

#include <fstream>
#include <sstream>

std::string get_program_log(GLuint object) {
    GLint length = 0;
    glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);

    std::string log;
    if(length > 0) {
        GLcharARB *compile_log = new GLcharARB[length];

        GLint characters_written = 0;
        glGetInfoLogARB(object, length, &characters_written, compile_log);

        log = compile_log;
        delete[] compile_log;
    }

    return log;
}

Shader::Shader(const GLenum &type) : shader_type(type) {
    shader_file     = "";
    shader_id       = 0;
    shader_compiled = false;
}

void Shader::load(const std::string &file) {
    shader_file = file;

    std::ifstream f(file);
    std::stringstream buffer;
    buffer << f.rdbuf();

    shader_contents = buffer.str();

    shader_compiled = false;
}

bool Shader::compile() {
    if(shader_id == 0) {
        shader_id = glCreateShader(shader_type);
    }

    if(shader_id && !shader_compiled) {
        const char *buffer = shader_contents.c_str();
        glShaderSource(shader_id, 1, &buffer, nullptr);
        glCompileShader(shader_id);

        GLint success;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);

        std::string log = get_program_log(shader_id);
        if(success == GL_FALSE) {
            printf("shader_file:%scompile log:\n%s", shader_file.c_str(), log.c_str());
            return false;
        } else {
            // printf("shader_file:%s\nwarnings:\n%s\n", shader_file.c_str(), log.c_str());
        }
    }

    return true;
}

void Shader::release() {
    if(shader_id) {
        glDeleteShader(shader_id);
    }
}

void Shader::attach(const GLuint &program) {
    if(shader_id == 0) {
        shader_id = glCreateShader(shader_type);
    }

    if(program && shader_id) {
        glAttachShader(program, shader_id);
    }
}

void Shader::detach(const GLuint &program) {
    if(program && shader_id) {
        glDetachShader(program, shader_id);
    }
}

VertexShader::VertexShader() : Shader(GL_VERTEX_SHADER) {}

FragmentShader::FragmentShader() : Shader(GL_FRAGMENT_SHADER) {}
