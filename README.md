# png2mesh

<img src="https://github.com/holke/png2mesh/blob/main/logo/png2mesh_logo_mesh.png?raw=true" width="400" height="400" />

The png2mesh library can convert `.png` images into adaptive meshes.
In order to do so, it uses the ![t8code](https://github.com/holke/t8code/) library to build a mesh and refine it at the positions of dark pixels in the original image.
The generated output are 2 `.pvtu` files that can be visualized with the ![paraview](https://www.paraview.org/) software. The first `.pvtu` file stores the original mesh, the second a 2:1 balanced version of that mesh.


Currently, the created mesh has quad elements. We are working on extending png2mesh to triangle elements.

# requirements

- t8code (develop branch); See ![t8code](https://github.com/holke/t8code/tree/develop)
- p4est (usually installed with t8code)
- sc    (usually installed with t8code)
- libpng
- MPI

optional: Paraview to visualize the resulting meshes.

# installation

Before installing ensure that t8code, p4est, sc and libpng are installed.
If t8code, p4est and sc are installed to non-standard directories,
the following environment variables must be set:

T8_INCLUDE -- Include directory of t8code, p4est and sc

T8_LIB -- Library directory containing the t8code, p4est and sc libraries.


png2mesh uses CMake. 

To build it on a linux environment, perform the following steps:

1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make`

# examples


You find some example .png files in the `examples/` folder.

To build the png2mesh logo from the file `examples/png2mesh_logo.png` call

`./png2mesh_demo -f examples/png2mesh_logo.png -m9`

and look at the created file `t8_png_balance_png2mesh_logo.png_Quad.pvtu` in Paraview.


To build a reasonable mesh from the heart.png you must set the threshold to >= 255:

`png2mesh_demo -t 255 -f examples/heart.png`

# usage

Use the program as `png2mesh_demo [OPTIONS]`, where OPTIONS can be any of the following command line parameters:

| Option        | Parameter     | Effect  |
|:------------- |:-------------|:-----|
| -h (--help)       | NONE      | Display a short help message. |
| -f (--file)       | FILENAME  | The input png file. |
| -i (--invert)     | NONE      | Invert the refinement (refine bright areas, not dark). |
| -l (--level)      | INT >= 0  | The initial refinement level of the mesh. Default 0. |
| -m (--maxlevel)   | INT >= 0  | The maximum allowed refinement level of the mesh. Default 10. |
| -t (--threshold)  | INT >= 0 and <= 3 * 255 | How sensitive the refinement reacts to RGB values. The mesh is refined in areas with red + green + blue < threshold. |
