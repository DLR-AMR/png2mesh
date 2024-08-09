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

        /* Read and check color depth. */

        const png_byte bit_depth = png_get_bit_depth(image->png_ptr, image->info_ptr);
        if (bit_depth != 8) {
                fprintf(stderr, "[png2mesh] ERROR: File %s has color bit depth %i, but png2mesh only supports 8. Maybe you can convert your file?\n", filename, bit_depth);
                free (image);
                return NULL;
        }


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
                printf ("Reading color type RGB.\n");
        }
        else if (image->color_type == PNG_COLOR_TYPE_RGBA) {
                image->num_values_per_pixel = 4;
                printf ("Reading color type RGBA.\n");
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

void png2mesh_print_png (const png2mesh_image_t *image)
{
        assert (image != NULL);
        assert (image->rgba_values != NULL);

        printf ("Printing image.\n");
        printf ("File:\t%s\n", image->filename);
        printf ("Size:\t%i x %i\n", image->width, image->height);
        printf ("Values pp:\t%i\n", image->num_values_per_pixel);

        if (image->width > 10 || image->height > 10) return; // Do not print large pictures.
        for (int y = 0;y < image->height;++y) {
                printf ("%i:\t", y);
                for (int x = 0;x < image->width;++x) {
                     png_byte *RGBA;
                     png2mesh_get_rgba (image, x, y, &RGBA);

                     printf ("(%i, %i, %i) ", RGBA[0], RGBA[1], RGBA[2]);
                     if (image->num_values_per_pixel == 4) {
                        printf ("[%i]  ", RGBA[3]);
                     }
                }
                printf ("\n");
        }
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
