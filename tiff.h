#ifndef _tiff_h_
#define _tiff_h_

#include <string>
#include <vector>

void write_tiff(const std::string &filename,
                const int &width,
                const int &height,
                const int &channels,
                const std::vector<uint8_t> &buffer);

#endif
