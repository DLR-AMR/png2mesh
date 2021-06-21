# png2mesh

![png2mesh](https://github.com/holke/png2mesh/blob/main/logo/png2mesh_logo_mesh.png?raw=true)

# requirements

- t8code
- p4est (usually installed with t8code)
- sc    (usually installed with t8code)
- libpng
- MPI (if t8code was build parallel)

# installation

On linux systems calling the compile.scp script should suffices.
Before calling it ensure that the t8code lib and include paths are set
properly in the script.

# examples

You find some example .png files in the examples/ folder.

For the heart.png you must set the threshold to >= 255:

png2mesh -t 255 -f examples/heart.png
