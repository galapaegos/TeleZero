#include "Program.h"

Program::Program(const std::string &vert, const std::string &frag) {
    fragment_shader      = std::make_unique<FragmentShader>();
    fragment_shader_file = frag;

    vertex_shader      = std::make_unique<VertexShader>();
    vertex_shader_file = vert;

    program_id   = 0;
    link_program = false;
}

void Program::create() {
    if(program_id == 0) {
        program_id = glCreateProgram();
    }

    vertex_shader->load(vertex_shader_file);
    fragment_shader->load(fragment_shader_file);
	check_error();

    vertex_shader->attach(program_id);
    fragment_shader->attach(program_id);
	check_error();

    if(!vertex_shader->compile())
        return;

    if(!fragment_shader->compile())
        return;
	check_error();
}

void Program::destroy() {
    if(program_id) {
        glDeleteProgram(program_id);
		check_error();
    }

    vertex_shader->release();
    fragment_shader->release();
	check_error();
}

void Program::bind() {
    if(!link_program) {
        glLinkProgram(program_id);
		check_error();

        GLint success = 0;
        glGetProgramiv(program_id, GL_LINK_STATUS, &success);
		check_error();

        std::string log = get_program_log(program_id);
        if(success == GL_FALSE) {
            printf("Unable to link programs [%s][%s]", vertex_shader_file.c_str(), fragment_shader_file.c_str());
            printf("Error:%s", log.c_str());
            link_program = false;
            return;
        } else
            link_program = true;
    }

    glUseProgram(program_id);
	check_error();

    int texture_id = 0;
    for(auto i = 0; i < textures.size(); i++) {
        set_uniform(textures[i].name, texture_id);

        glActiveTexture(GL_TEXTURE0 + texture_id);

        textures[i].value->bind();

        texture_id++;
    }
	check_error();
}

void Program::unbind() {
    size_t texture_id = textures.size() - 1;
    for(int i = int(texture_id); i >= 0; i--) {
        glActiveTexture(GL_TEXTURE0 + GLenum(texture_id));
        textures[i].value->unbind();
    }
	check_error();

    glUseProgram(0);
	check_error();
}

void Program::bind_parameter(const std::string &name, std::shared_ptr<Texture> tex) {
    TextureItem item;
    item.name  = name;
    item.value = tex;

    textures.push_back(item);
}

void Program::set_uniform(const std::string &loc, const int &value) {
    GLint position = get_uniform_location(loc);
    glUniform1i(position, value);
	check_error();
}
