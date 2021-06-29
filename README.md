# png2mesh

![png2mesh](https://github.com/holke/png2mesh/blob/main/logo/png2mesh_logo_mesh.png?raw=true)

# requirements

- t8code (develop branch); See ![t8code](https://github.com/holke/t8code/tree/develop)
- p4est (usually installed with t8code)
- sc    (usually installed with t8code)
- libpng
- MPI

# installation

Before installing ensure that t8code, p4est, sc and libpng are installed.
If t8code, p4est and sc are installed to non-standard directories,
the following environment variables must be set:

T8_INCLUDE -- Include directory of t8code, p4est and sc
T8_LIB -- Library directory containing the t8code, p4est and sc libraries.

# examples

You find some example .png files in the examples/ folder.

For the heart.png you must set the threshold to >= 255:

png2mesh -t 255 -f examples/heart.png
