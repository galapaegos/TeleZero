#include "glcheck.h"

#include <GL/glew.h>

void print_gl_error(const std::string &file, const int &line) {
	GLenum error = glGetError();
	
	while(error != GL_NO_ERROR) {
		printf("OpenGL error: %i; %s - %s:%i\n", error, gluErrorString(error), file.c_str(), line);
		error = glGetError();
	}
}
