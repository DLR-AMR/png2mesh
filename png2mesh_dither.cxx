#include <assert.h>
#include <vector>
#include <iostream>
#include <filesystem>
#include "png2mesh_dither.hxx"

void
png2mesh_apply_dither (png2mesh_image_t * image, const enum DITHER_MODE dither_mode, const int dither_per_subgrid, const int user_threshold)
{
  assert (dither_per_subgrid > 0);

  // Define the dither thresholds for 2x2,, 4x4, or 8x8 dither.
  // All the dither values are computed for 8 bit depth. (0 <= v < 256)
  static const int dither_thresholds_2x2[2][2] = {
    {0, 128},
    {192, 64}
  };

  static const int dither_thresholds_4x4[4][4] = {
    {0, 128, 32, 160},
    {192, 64, 224, 96},
    {48, 176, 16, 144},
    {240, 112, 208, 80},
  };

 static const int dither_thresholds_8x8[8][8] = {
    {0,128,32,160,8,136,40,168},
    {192,64,224,96,200,72,232,104},
    {48,176,16,144,56,184,24,152},
    {240, 112,208,80,248,120,216,88},
    {12,140,44,172,4,132,36,164},
    {204,76,236,108,196,68,228,100},
    {60,188,28,156,52,180,20,148},
    {252,124,220,92,244,116,212,84}
  };

  // use a temp var to make actual variable const after setting it.
  int temp_dither_length;

  switch (dither_mode) {
    case TWO_BY_TWO:
      temp_dither_length = 2;
      break;
    case FOUR_BY_FOUR:
      temp_dither_length = 4;
      break;
    case EIGHT_BY_EIGTH:
      temp_dither_length = 8;
      break;
    default:
      fprintf (stderr, "ERROR: Invalid dither mode.\n");
      return;
  }

  const int dither_length = temp_dither_length; // used temp var to make actual variable const after setting.
  const int subgrid_size = dither_per_subgrid * dither_length;

  std::vector<std::vector<int>> thresholds (subgrid_size, std::vector<int> (subgrid_size));
 
  // Compute the dither thresholds
  switch (dither_mode) {
    case TWO_BY_TWO:
      png2mesh_compute_thresholds (dither_thresholds_2x2, thresholds, subgrid_size, dither_per_subgrid);
      break;
    case FOUR_BY_FOUR:
      png2mesh_compute_thresholds (dither_thresholds_4x4, thresholds, subgrid_size, dither_per_subgrid);
      break;
    case EIGHT_BY_EIGTH:
      png2mesh_compute_thresholds (dither_thresholds_8x8, thresholds, subgrid_size, dither_per_subgrid);
      break;
    default:
      fprintf (stderr, "ERROR: Invalid dither mode.\n");
      return;
  }

  // Depending on user threshold and dither threshold make each pixels in the png
  // black or white.
  const int user_threshold_scaled = user_threshold / 3;

  for (int x = 0; x < image->width / subgrid_size;++x) { // Iterate over x subgrids
    for (int y = 0; y < image->height / subgrid_size;++y) { // Iterate over y subgrids
      for (int subgrid_x = 0;subgrid_x < subgrid_size; ++subgrid_x) { // Iterate x within subgrid
        for (int subgrid_y = 0;subgrid_y < subgrid_size; ++subgrid_y) { // Iterate y withing subgrid
          png_byte *pixel;
          png2mesh_get_rgba (image, x * subgrid_size + subgrid_x, y * subgrid_size + subgrid_y, &pixel);
          const int brightness = (pixel[0] + pixel[1] + pixel[2]) / 3;
          if (brightness > std::min(user_threshold_scaled, thresholds[subgrid_x][subgrid_y])) {
            // bright pixel
            png2mesh_set_pixel_bw (pixel, 1);
          }
          else {
            // dark pixel
            png2mesh_set_pixel_bw (pixel, 0);
          }
        }
      }
    }
  }
}