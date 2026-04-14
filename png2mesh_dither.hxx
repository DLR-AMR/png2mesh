#ifndef PNG2MESH_DITHER_H
#define PNG2MESH_DITHER_H

#include "png2mesh_readpng.h"

/* In this file we define functions to apply a dithering filter.
 See also: https://beltoforion.de/de/dithering/ for a German introduction.
 Dithering can improve the AMR effect of the image.
 It sets each pixel in the image to either white or black depending on a specific dither stencil matrix.
 This effectively generates the impression of greyscales even though only 2 colors (black, white) are used.
 We support Bayer dithering with 2x2, 4x4, 8x8 stencils.
 A dither_per_subgrid parameter can further "Coarsen" the dither effect, which might lead to better results.
 */

/* Dither mode, controls the size of the dither stencil */
enum DITHER_MODE {
  TWO_BY_TWO,    
  FOUR_BY_FOUR,
  EIGHT_BY_EIGTH
};

/* Apply the dither effect to a png image.
 * This will change the color pixels to be black and white only.
 \param [in,out] image A png image, 8bit depth, RGBA. On output, each pixel will be white or black.
 \param [in] dither_mode. The size of the dither stencil and the number of perceived grey values.
 \param [in] dither_per_subgrid. Factor to blow-up the size of the dither stencil. See below.
 \param [in] user_threshold. Controls the sensitivity of refinement. Any pixels above this threshold will be set to white regardless of the dither value.

 Example for dither_per_subgrid, and a TWO_BY_TWO dither_mode.
 The dither stencil is
 0  128
 192 64
 
 With dither_per_subgrid = 1 pixel 0,0 is compared to brightness 0, pixel 0,1 to 128, pixel 1,0 to 192, pixel 1,1 to 64.
 With dither_per_subgrid = 2, the stencil is blown up to 2x its size and becomes:

   0   0 128 128
   0   0 128 128
 192 192  64  64
 192 192  64  64
*/
void png2mesh_apply_dither (png2mesh_image_t * image, const enum DITHER_MODE dither_mode, const int dither_per_subgrid, const int user_threshold);

template <size_t N>
void png2mesh_compute_thresholds (const int (&dither_thresholds)[N][N],
                                  std::vector<std::vector<int>> &thresholds,
                                  const int subgrid_size, const int dither_per_subgrid)
{
  for (int thresh_x = 0;thresh_x < subgrid_size;++thresh_x) {
    for (int thresh_y = 0;thresh_y < subgrid_size;++thresh_y) {
      // Convert x from 0 <= x < subgrid_size to 0 <= x < dither_length
      // Convert y from 0 <= y < subgrid_size to 0 <= y < dither_length
      // -> x,y / dither_per_subgrid 
      const int dither_value = dither_thresholds[thresh_x / dither_per_subgrid][thresh_y / dither_per_subgrid];
      thresholds[thresh_x][thresh_y] = dither_value;
      // std::cout << dither_value << " "; // Debug printing values
    }
     // std::cout << std::endl; // Debug printing values
  }
}

#endif
