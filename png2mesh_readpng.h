#ifndef PNG2MESH_READPNG_H
#define PNG2MESH_READPNG_H

#include <png.h>

typedef struct
{
    int width, height;
    int num_values_per_pixel;
    png_structp png_ptr;
	png_infop info_ptr;
    png_bytep *rgba_values;
    png_byte  color_type;
    const char *filename;
    /* data */
} png2mesh_image_t;

#ifdef __cplusplus
extern "C" {
#endif


png2mesh_image_t *png2mesh_read_png(const char* file_name);
void png2mesh_get_rgba(const png2mesh_image_t *image, const int x, const int y, png_byte **RGBA);
void png2mesh_image_cleanup (png2mesh_image_t *mypng);
void png2mesh_print_png (const png2mesh_image_t *image);

#ifdef __cplusplus
}
#endif

#endif
