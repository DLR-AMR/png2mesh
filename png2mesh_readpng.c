/*
 * This file was inspired by
 * an original from Guillaume Cottenceau.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#define PNG_DEBUG 3
#include "png2mesh_readpng.h"

	

png2mesh_image_t *png2mesh_read_png(const char* filename)
{
	png2mesh_image_t *image = (png2mesh_image_t*) malloc (sizeof(png2mesh_image_t));
        int rowbytes;
        int bytes_read;

        if (image == NULL) {
                return NULL;
        }
        unsigned char header[8];

        /* open file and test for it being a png */
        FILE *fp = fopen(filename, "rb");
        if (fp == NULL) {
                fprintf(stderr, "[png2mesh] ERROR: Could not open file %s.\n", filename);
                free (image);
                return NULL;
        }
        /* Read the header */
        bytes_read = fread(header, 1, 8, fp);
        /* Chech that header belongs to png file */
        if (bytes_read != 8 || png_sig_cmp(header, 0, 8)) {
                fprintf(stderr,"[png2mesh] ERROR: File %s is not a PNG file.\n", filename);
                free (image);
                return NULL;
        }


        /* initialize png pointer */
        image->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (image->png_ptr ==NULL) {
                fprintf(stderr, "[png2mesh] ERROR: Could not read png struct from file %s.\n", filename);
                free (image);
                return NULL;
        }

        /* Read png info struct */
        image->info_ptr = png_create_info_struct(image->png_ptr);
        if (image->info_ptr == NULL) {
                fprintf(stderr, "[png2mesh] ERROR: Could not create info struct from file %s.\n", filename);
                free (image);
                return NULL;
        }
        
        /* Set jump to png jumpbuf */
        if (setjmp(png_jmpbuf(image->png_ptr))) {
                fprintf(stderr, "[png2mesh] ERROR: Could not set jump for png from file %s.\n", filename);
                free (image);
                return NULL;
        }

        /* Init png io and set sig bytes */
        png_init_io(image->png_ptr, fp);
        png_set_sig_bytes(image->png_ptr, 8);

        /* Read the png info */
        png_read_info(image->png_ptr, image->info_ptr);

        /* Read width and height of image */
        image->width = png_get_image_width(image->png_ptr, image->info_ptr);
        image->height = png_get_image_height(image->png_ptr, image->info_ptr);
       
        /* set jump buffer */
        if (setjmp(png_jmpbuf(image->png_ptr))) {
                fprintf(stderr, "[png2mesh] ERROR: Could not set jump for png from file %s.\n", filename);
                free (image);
                return NULL;
        }

        rowbytes = png_get_rowbytes(image->png_ptr,image->info_ptr);
        image->color_type = png_get_color_type(image->png_ptr, image->info_ptr);
        if (image->color_type == PNG_COLOR_TYPE_RGB) {
                image->num_values_per_pixel = 3;
        }
        else if (image->color_type == PNG_COLOR_TYPE_RGBA) {
                image->num_values_per_pixel = 4;
        }
        else {
                fprintf(stderr, "[png2mesh] ERROR: Color type of %s is not supported.\n", filename);
                free (image);
                return NULL;
        }

        /* Allocate rgb pointers */
        image->rgba_values = (png_bytep*) malloc(image->height * sizeof(png_bytep));
        if (image->rgba_values == NULL) {
                fprintf(stderr, "[png2mesh] ERROR: Memory allocation failed.\n");
                free (image);
                return NULL;
        }
        for (int y=0; y < image->height; y++) {
                image->rgba_values[y] = (png_byte*) malloc(rowbytes);
        }

        png_read_image(image->png_ptr, image->rgba_values);


        fclose(fp);

        image->filename = filename;
        return image;
}

void png2mesh_get_rgba(const png2mesh_image_t *image, const int x, const int y, png_byte **RGBA)
{
        assert (0 <= x && x < image->width);
        assert (0 <= y && y < image->height);
        assert (image != NULL);
        assert (image->rgba_values != NULL);
      
        *RGBA = &image->rgba_values[y][image->num_values_per_pixel * x];
}

void png2mesh_image_cleanup (png2mesh_image_t *image)
{
        free (image->rgba_values);
        png_destroy_read_struct(&image->png_ptr, &image->info_ptr, NULL);
}
