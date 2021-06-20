#include "png2mesh_readpng.h"

int main () {
    png2mesh_image_t *pngimage;
    const char *filename = "./examples/heart.png";

	pngimage = png2mesh_read_png (filename);
    if (pngimage == NULL) {
        fprintf (stderr, "ERROR: Could not read image %s.\n", filename);
        return 1;
    }
    printf ("Read image from %s\n", filename);
    printf ("height: %i\nwidth:  %i\n", pngimage->height, pngimage->width);

	png2mesh_image_cleanup (pngimage);

    return 0;
}
