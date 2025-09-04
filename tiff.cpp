#include "tiff.h"

#include <tiffio.h>

void write_tiff(const std::string &filename,
                const int &width,
                const int &height,
                const int &channels,
                const std::vector<uint8_t> &buffer) {
    TIFF *tif = TIFFOpen(filename.c_str(), "w");
    if(!tif) {
        printf("Unable to open tiff file for writing (%s)\n", filename.c_str());
        return;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, channels);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    int rows_per_strip = int(65536 / width);
    if(rows_per_strip == 0) {
        rows_per_strip = 1;
    }
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip);

    unsigned short extyp = EXTRASAMPLE_ASSOCALPHA;
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &extyp);

    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buffer.data());
    const int jump     = width * channels;
    for(uint32_t y = 0; y < height; y++) {
        if(TIFFWriteScanline(tif, (void *)(ptr + y * jump), y, 0) == -1) {
            TIFFClose(tif);
            printf("Unable to write scanline for tiff\n");
            return;
        }
    }

    TIFFWriteDirectory(tif);

    TIFFClose(tif);
}
