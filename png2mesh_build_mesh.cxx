#include <libgen.h>
#include <sc_options.h>
#include <t8.h>
#include <t8_forest.h>
#include <t8_cmesh.h>
#include <t8_schemes/t8_default_cxx.hxx>
#include <assert.h>
#include "png2mesh_readpng.h"

typedef struct {
	const png2mesh_image_t *image;
	int		maxlevel; /* maximum allowed refinement level */
	int		threshold; /* r+g+b threshold for refinement. 0 <= values <= 3*255 */
} png2mesh_adapt_context_t;

int png2mesh_element_has_dark_pixel (t8_forest_t forest, t8_locidx_t ltreeid, const t8_element_t *element, t8_eclass_scheme_c *scheme, 
									 const double *tree_vertices, const png2mesh_image_t *image,
									 const int dark_threshold)
{
	double coords_downleft[3];
	double coords_upright[3];
	int x_start, x_end, y_start, y_end;
	int ix, iy;
	png_byte *pixel = NULL;
	t8_element_shape_t element_shape;
	int second_vertex;
	assert (tree_vertices != NULL);
	assert (image != NULL);
	assert (image->rgba_values != NULL);

	element_shape = scheme->t8_element_shape (element);
	assert (element_shape == T8_ECLASS_TRIANGLE || element_shape == T8_ECLASS_QUAD);
	second_vertex = element_shape == T8_ECLASS_QUAD ? 3 : 2;
	t8_forest_element_coordinate (forest, ltreeid, element, tree_vertices, 0, coords_downleft);
	t8_forest_element_coordinate (forest, ltreeid, element, tree_vertices, second_vertex, coords_upright);
	/* Scale coords by width and height of png */
	coords_downleft[0] *= image->width;
	coords_downleft[1] *= image->height;
	coords_upright[0] *= image->width;
	coords_upright[1] *= image->height;
	/* compute x_start, x_end, y_start, y_end.
	 * the window of pixel coordinates covered by the image.
	 * We need to flip the y coordinate. */
	x_start = SC_MIN(coords_downleft[0], coords_upright[0]);
	x_end = SC_MAX(coords_downleft[0], coords_upright[0]);
	y_start = SC_MIN(image->height - coords_downleft[1], image->height - coords_upright[1]);
	y_end = SC_MAX(image->height - coords_downleft[1], image->height - coords_upright[1]);
	if (x_end - x_start < 1 && y_end - y_start < 1) {
		/* The element is smaller than a single pixel.
		 * It does not contain a dark pixel by definition. */
		return 0;
	}
	/* Iterate over all pixels covered by the element and check
	 * if any one is dark. */
	for (ix = x_start;ix < x_end;++ix) {
		for (iy = y_start;iy < y_end;++iy) {
			png2mesh_get_rgba (image, ix, iy, &pixel);
			if (pixel[0] + pixel[1] + pixel[2] <= dark_threshold) {
				return 1;
			}
		}
	}
	/* No dark pixels found. */
	return 0;
}

int
png2mesh_adapt (t8_forest_t forest,
                         t8_forest_t forest_from,
                         t8_locidx_t which_tree,
                         t8_locidx_t lelement_id,
                         t8_eclass_scheme_c * ts,
                         int num_elements, t8_element_t * elements[])
{
	const png2mesh_adapt_context_t *ctx = (const png2mesh_adapt_context_t*) t8_forest_get_user_data (forest);
	const png2mesh_image_t *image = ctx->image;
	const int maxlevel = ctx->maxlevel;
	const double * tree_vertices = t8_forest_get_tree_vertices (forest_from, which_tree);

	if (ts->t8_element_level (elements[0]) >= maxlevel) {
		return 0;
	}

	return png2mesh_element_has_dark_pixel (forest_from, which_tree, elements[0], ts, tree_vertices, image, ctx->threshold);
}

void build_forest (int level, t8_eclass_t element_class, sc_MPI_Comm comm, const png2mesh_adapt_context_t *adapt_context)
{
	t8_scheme_cxx_t *scheme = t8_scheme_new_default_cxx ();

	t8_cmesh_t cmesh = t8_cmesh_new_hypercube (element_class, sc_MPI_COMM_WORLD, 0, 0, 0);
	t8_forest_t forest = t8_forest_new_uniform (cmesh, scheme, level, 0, comm);
	t8_forest_t forest_adapt = t8_forest_new_adapt (forest, png2mesh_adapt, 1, 0, (void*) adapt_context);
	t8_forest_t forest_balance;
	char vtuname[BUFSIZ];

	snprintf (vtuname, BUFSIZ, "t8_png_adapt_%s_%s", basename ( (char*)adapt_context->image->filename), t8_eclass_to_string[element_class]);
	t8_forest_write_vtk (forest_adapt, vtuname);

	t8_forest_init (&forest_balance);
	t8_forest_set_balance (forest_balance, forest_adapt, 0);
	t8_forest_commit (forest_balance);

	snprintf (vtuname, BUFSIZ, "t8_png_balance_%s_%s", basename ((char*)adapt_context->image->filename), t8_eclass_to_string[element_class]);
	t8_forest_write_vtk (forest_balance, vtuname);

	printf ("\nSuccessfully build AMR mesh for picture %s.\n", adapt_context->image->filename);
	printf ("Width:  %i px\nHeight: %i px\n", adapt_context->image->width, adapt_context->image->height);

	t8_forest_unref (&forest_balance);
}

int main (int argc, char *argv[])
{
	int mpiret;
	int level = 0;
	int maxlevel = 0;
	int helpme = 0;
	int parsed = 0;
	int threshold;
	int element_shape = 0;
	t8_eclass_t element_class;
	png2mesh_image_t *pngimage;
	png2mesh_adapt_context_t adapt_context;
	sc_options_t *opt;
	const char *filename;
	const char *help = "The program reads a png file and builds an adaptive mesh from it.\n"
					   " The mesh is refined in the dark regions of the image.";

	/* Initialize MPI. This has to happen before we initialize sc or t8code. */
	mpiret = sc_MPI_Init (&argc, &argv);
	/* Error check the MPI return value. */
	SC_CHECK_MPI (mpiret);
	
	/* Initialize the sc library, has to happen before we initialize t8code. */
	sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
	/* Initialize t8code with log level SC_LP_PRODUCTION. See sc.h for more info on the leg levels. */
	t8_init (SC_LP_PRODUCTION);
	

	/* initialize command line argument parser */
	opt = sc_options_new (argv[0]);
	sc_options_add_switch (opt, 'h', "help", &helpme,
							"Display a short help message.");
	sc_options_add_string (opt, 'f', "file", &filename, "",
						"png file.");
	sc_options_add_int (opt, 'l', "level", &level, 0,
						"The initial refinement level of the mesh.");
	sc_options_add_int (opt, 'm', "maxlevel", &maxlevel, 10,
						"The maximum allowed refinement level of the mesh.");
	sc_options_add_int (opt, 't', "threshold", &threshold, 100,
						"How sensitive the refinement reacts to RGB values.\n"
						"\t\t\t\t\tThe mesh is refined in areas with red + green + blue < threshold.\n"
						"\t\t\t\t\tValues must be between 0 and 3*255.");
#if 0 /* Currently deactived. Reactive when triangles are working. */
	sc_options_add_int (opt, 'e', "element_shape", &element_shape, 0,
						"The shape of elements to use. 0: quad, 1: triangle");
#endif

	parsed =
		sc_options_parse (-1, SC_LP_ERROR, opt, argc, argv);
	if (helpme) {
		/* display help message and usage */
		t8_global_productionf ("%s\n", help);
		sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
	}
	else if (parsed >= 0 && 0 <= level && strcmp (filename, "") && 
	    (element_shape == 0 || element_shape == 1)
		&& level <= maxlevel && 0 <= threshold && threshold <= 3 * 255) {
		pngimage = png2mesh_read_png (filename);
		element_class = element_shape == 0 ? T8_ECLASS_QUAD : T8_ECLASS_TRIANGLE;
		if (pngimage != NULL) {
			adapt_context.image = pngimage;
			adapt_context.maxlevel = maxlevel;
			adapt_context.threshold = threshold;
			build_forest (level, element_class, sc_MPI_COMM_WORLD, &adapt_context);
			png2mesh_image_cleanup (pngimage);
		}
	}
	else {
		/* wrong usage */
		t8_global_productionf ("\n\t ERROR: Wrong usage.\n\n");
		sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
	}

	sc_options_destroy (opt);

	sc_finalize ();

	mpiret = sc_MPI_Finalize ();
	SC_CHECK_MPI (mpiret);
	return 0;
}

