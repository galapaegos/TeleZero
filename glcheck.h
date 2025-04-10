#ifndef _glcheck_h_
#define _glcheck_h_

#include <string>
#include <vector>

void print_gl_error(const std::string &file, const int &line);
#define check_error() print_gl_error(__FILE__, __LINE__);

#endif 

