
#include <libgen.h>
#include <sc_options.h>
#include <t8.h>
#include <t8_forest/t8_forest.h>
#include <t8_forest/t8_forest_iterate.h>
#include <t8_cmesh.h>
#include <t8_cmesh/t8_cmesh_examples.h>
#include <t8_schemes/t8_default/t8_default.hxx>
#include <assert.h>
#include "png2mesh_readpng.h"

#if 0
typedef struct
{
  const png2mesh_image_t *image;
  int                 maxlevel; /* maximum allowed refinement level */
  int                 threshold;        /* r+g+b threshold for refinement. 0 <= values <= 3*255 */
  bool                invert;   /* If true, refine bright areas, not dark. */
  sc_array_t          refinement_markers;       /* For each element 1 if it should be refined, 0 if not. */
} png2mesh_adapt_context_t;

int
png2mesh_pixel_match (const png2mesh_image_t * image, const int pixel_x,
                      const int pixel_y, const bool invert,
                      const int dark_threshold)
{
  png_byte           *pixel = NULL;
  png2mesh_get_rgba (image, pixel_x, pixel_y, &pixel);
  int                 pixel_sum = pixel[0] + pixel[1] + pixel[2];
  if (!invert) {
    if (pixel_sum <= dark_threshold) {
      return 1;
    }
  }
  else {
    if (pixel_sum >= dark_threshold) {
      return 1;
    }
  }
  return 0;
}

int
png2mesh_element_has_dark_pixel (t8_forest_t forest, t8_locidx_t ltreeid,
                                 const t8_element_t *element,
                                 t8_eclass_scheme_c *scheme,
                                 const double *tree_vertices,
                                 const png2mesh_image_t * image,
                                 const int dark_threshold, const bool invert)
{
  double              coords_downleft[3];
  double              coords_upright[3];
  int                 x_start, x_end, y_start, y_end;
  int                 ix, iy;
  t8_element_shape_t  element_shape;
  int                 second_vertex;
  assert (tree_vertices != NULL);
  assert (image != NULL);
  assert (image->rgba_values != NULL);

  element_shape = scheme->t8_element_shape (element);
  assert (element_shape == T8_ECLASS_TRIANGLE
          || element_shape == T8_ECLASS_QUAD);
  second_vertex = element_shape == T8_ECLASS_QUAD ? 3 : 2;
  t8_forest_element_coordinate (forest, ltreeid, element, 0, coords_downleft);
  t8_forest_element_coordinate (forest, ltreeid, element, second_vertex,
                                coords_upright);
  /* Scale coords by width and height of png */
  coords_downleft[0] *= image->width;
  coords_downleft[1] *= image->height;
  coords_upright[0] *= image->width;
  coords_upright[1] *= image->height;
  /* compute x_start, x_end, y_start, y_end.
   * the window of pixel coordinates covered by the image.
   * We need to flip the y coordinate. */
  x_start = SC_MIN (coords_downleft[0], coords_upright[0]);
  x_end = SC_MAX (coords_downleft[0], coords_upright[0]);
  y_start =
    SC_MIN (image->height - coords_downleft[1],
            image->height - coords_upright[1]);
  y_end =
    SC_MAX (image->height - coords_downleft[1],
            image->height - coords_upright[1]);
  if (x_end - x_start < 1 && y_end - y_start < 1) {
    /* The element is smaller than a single pixel.
     * It does not contain a dark pixel by definition. */
    return 0;
  }
  /* Iterate over all pixels covered by the element and check
   * if any one is dark. */
  for (ix = x_start; ix < x_end; ++ix) {
    for (iy = y_start; iy < y_end; ++iy) {
      if (png2mesh_pixel_match (image, ix, iy, invert, dark_threshold)) {
        return 1;
      }
    }
  }
  /* No dark pixels found. */
  return 0;
}

int
png2mesh_search_callback (t8_forest_t forest,
                          const t8_locidx_t ltreeid,
                          const t8_element_t *element,
                          const int is_leaf,
                          const t8_element_array_t *leaf_elements,
                          const t8_locidx_t
                          tree_leaf_index, void *query, sc_array_t *query_indices,
                          int *query_matches, const size_t num_active_queries)
{
  printf ("Num active = %i\n", num_active_queries);
  if (query != NULL) {

    const png2mesh_adapt_context_t *ctx =
      (const png2mesh_adapt_context_t *) t8_forest_get_user_data (forest);
    /* Compute for each query whether it is inside the element or not.
     * We do this in batch for all queries at the same time. */
    int *is_inside = T8_ALLOC (int, num_active_queries); // Allocate memory for the is_inside return values
    double *pixel_scaled_coords = T8_ALLOC (double, 3*num_active_queries); // Allocate memory for the queries coordinates
    for (size_t iquery = 0;iquery < num_active_queries;++iquery) {
      const int           query_value = *(int *) sc_array_index ((sc_array_t*)query, iquery);
      const int           pixel_x = query_value % ctx->image->width;
      const int           pixel_y = query_value / ctx->image->width;

      assert (0 <= pixel_x && pixel_x < ctx->image->width);
      assert (0 <= pixel_y && pixel_y < ctx->image->height);

      pixel_scaled_coords[3 * iquery] = pixel_x / (double) ctx->image->width;
      pixel_scaled_coords[3 * iquery + 1] = 1 - pixel_y / (double) ctx->image->height;
      pixel_scaled_coords[3 * iquery + 2] = 0;

    }

    t8_forest_element_points_inside
        (forest, ltreeid, element, pixel_scaled_coords, num_active_queries, is_inside, 1e-10);
    T8_FREE (pixel_scaled_coords);

    for (size_t iquery = 0;iquery < num_active_queries;++iquery) {
      const int           query_value = *(int *) sc_array_index ((sc_array_t*)query, iquery);
      const int           pixel_x = query_value % ctx->image->width;
      const int           pixel_y = query_value / ctx->image->width;


      query_matches[iquery] = 0;

      if (is_inside[iquery]) {
        /* This pixel is contained in the element */
        if (png2mesh_pixel_match
            (ctx->image, pixel_x, pixel_y, ctx->invert, ctx->threshold)) {
          /* This pixel is a pixel for which we refine elements. */
          query_matches[iquery] = 1;
          if (is_leaf) {
            printf ("Pixel %i %i is in element. Mark for refinement\n", pixel_x, pixel_y);
            /* We mark this element for later refinement. */
            const t8_locidx_t   element_index =
              tree_leaf_index + t8_forest_get_tree_element_offset (forest,
                                                                  ltreeid);
            *(int *) t8_sc_array_index_locidx ((sc_array_t *)
                                              &ctx->refinement_markers,
                                              element_index) = 1;
            /* We can end the search recursion here since we have found one matching pixel and will refine the element. */
            T8_FREE (is_inside);
            return 1;
          }
        }
      }
    }
    T8_FREE (is_inside);
  }
  else {                        /* query == NULL */
    return 1;
  }
  return 0;
}

int
png2mesh_adapt (t8_forest_t forest,
                t8_forest_t forest_from,
                t8_locidx_t which_tree,
                t8_locidx_t lelement_id,
                t8_eclass_scheme_c *ts,
                const int is_family,
                const int num_elements, t8_element_t *elements[])
{
  const png2mesh_adapt_context_t *ctx =
    (const png2mesh_adapt_context_t *) t8_forest_get_user_data (forest_from);
  const png2mesh_image_t *image = ctx->image;
  const int           maxlevel = ctx->maxlevel;
  const double       *tree_vertices =
    t8_forest_get_tree_vertices (forest_from, which_tree);
  const t8_locidx_t   element_index =
    lelement_id + t8_forest_get_tree_element_offset (forest_from, which_tree);

  if (ts->t8_element_level (elements[0]) >= maxlevel) {
    /* We do not refine if the element's level exceeds the provided maximum level */
    return 0;
  }
  /* Check whether this element is marked for refinement and if so,
   * refine it. */
  const int           element_marker =
    *(int *) t8_sc_array_index_locidx ((sc_array_t *)
                                       &ctx->refinement_markers,
                                       element_index);
  if (element_marker) {
    return 1;
  }
  return 0;
}

/* Build the array of queries.
 * This array contains all pixels that match the condition.
 * The pixels are encoded as an integer p with
 * x = p % image->width
 * y = p / image->width
 */
void
png2mesh_build_query_array (sc_array_t *queries,
                            const png2mesh_adapt_context_t * adapt_context)
{
  int                 ipixel;
  const png2mesh_image_t *image = adapt_context->image;
  const int           num_pixels = image->width * image->height;
  int                 array_entry;

  sc_array_init (queries, sizeof (int));
  for (ipixel = 0; ipixel < num_pixels; ++ipixel) {
    const int           x = ipixel % image->width;
    const int           y = ipixel / image->width;

    if (png2mesh_pixel_match
        (image, x, y, adapt_context->invert, adapt_context->threshold)) {
      /* This pixel matches, we insert it as query */
      *(int *) sc_array_push (queries) = ipixel;
    }
  }
  printf ("Build search array with %zd pixels (of %i)\n", queries->elem_count,
          num_pixels);
}

/*
 * element_choice: 0 - quad
 *				   1 - triangle
 *				   2 - quad/tri hybrid
 */
void
build_forest (int level, int element_choice, sc_MPI_Comm comm,
              const png2mesh_adapt_context_t * adapt_context)
{
  t8_scheme_cxx_t    *scheme = t8_scheme_new_default_cxx ();

  t8_cmesh_t          cmesh;
  char                element_string[BUFSIZ];
  int                 sreturn;
  if (element_choice < 2) {
    /* Build quad or triangle square */
    const t8_eclass_t   element_class =
      element_choice == 0 ? T8_ECLASS_QUAD : T8_ECLASS_TRIANGLE;
    cmesh =
      t8_cmesh_new_hypercube (element_class, sc_MPI_COMM_WORLD, 0, 0, 0);
    sreturn =
      snprintf (element_string, BUFSIZ, "%s",
                t8_eclass_to_string[element_class]);
  }
  else {
    T8_ASSERT (element_choice == 3);
    cmesh = t8_cmesh_new_periodic_hybrid (comm);
    sreturn = snprintf (element_string, BUFSIZ, "quadtrihybrid");
  }
  if (sreturn >= BUFSIZ) {
    /* String was truncated. */
    /* Note: gcc >= 7.1 prints a warning if we 
     * do not check the return value of snprintf. */
    t8_debugf ("Warning: Truncated output string to '%s'\n", element_string);
  }
  t8_forest_t         forest =
    t8_forest_new_uniform (cmesh, scheme, level, 0, comm);
  t8_forest_t         forest_adapt;     // = t8_forest_new_adapt (forest, png2mesh_adapt, 1, 0, (void*) adapt_context);
  t8_forest_t         forest_balance;
  t8_forest_t         forest_partition;
  sc_array_t          search_queries;
  char                vtuname[BUFSIZ];
  int                 ilevel;

  sc_array_init ((sc_array_t *) &adapt_context->refinement_markers,
                 sizeof (int));
  png2mesh_build_query_array (&search_queries, adapt_context);
  for (ilevel = level; ilevel < adapt_context->maxlevel; ++ilevel) {
    t8_forest_set_user_data (forest, (void *) adapt_context);
    printf ("Starting search on level %i\n", ilevel);
    /* Fill adapt markers array */
    const t8_locidx_t   num_elements =
      t8_forest_get_local_num_elements (forest);
    t8_locidx_t         ielement;
    sc_array_resize ((sc_array_t *) &adapt_context->refinement_markers,
                     num_elements);
    for (ielement = 0; ielement < num_elements; ++ielement) {
      /* Set all refinement markers to 0. */
      *(int *) t8_sc_array_index_locidx ((sc_array_t *)
                                         &adapt_context->refinement_markers,
                                         ielement) = 0;
    }
    /* Search and create the refinement markers. */
    t8_forest_search (forest, png2mesh_search_callback,
                      png2mesh_search_callback, &search_queries);
    t8_forest_init (&forest_adapt);
    t8_forest_set_adapt (forest_adapt, forest, png2mesh_adapt, 0);
    t8_forest_set_partition (forest_adapt, forest, 0);
    t8_forest_commit (forest_adapt);
    forest = forest_adapt;
  }
  sc_array_reset ((sc_array_t *) &adapt_context->refinement_markers);
  sc_array_reset (&search_queries);

  sreturn =
    snprintf (vtuname, BUFSIZ, "t8_png_adapt_%s_%s_t%i",
              basename ((char *) adapt_context->image->filename),
              element_string, adapt_context->threshold);
  if (sreturn >= BUFSIZ) {
    /* String was truncated. */
    /* Note: gcc >= 7.1 prints a warning if we 
     * do not check the return value of snprintf. */
    t8_debugf ("Warning: Truncated output string to '%s'\n", vtuname);
  }
  t8_forest_write_vtk (forest, vtuname);

  t8_forest_init (&forest_balance);
  t8_forest_set_balance (forest_balance, forest, 0);
  t8_forest_commit (forest_balance);

  sreturn =
    snprintf (vtuname, BUFSIZ, "t8_png_balance_%s_%s_t%i",
              basename ((char *) adapt_context->image->filename),
              element_string, adapt_context->threshold);
  if (sreturn >= BUFSIZ) {
    /* String was truncated. */
    /* Note: gcc >= 7.1 prints a warning if we 
     * do not check the return value of snprintf. */
    t8_debugf ("Warning: Truncated output string to '%s'\n", vtuname);
  }
  t8_forest_write_vtk (forest_balance, vtuname);

  printf ("\nSuccessfully build AMR mesh for picture %s.\n",
          adapt_context->image->filename);
  printf ("Width:  %i px\nHeight: %i px\n", adapt_context->image->width,
          adapt_context->image->height);

  t8_forest_unref (&forest_balance);
}
#endif

int
main (int argc, char *argv[])
{
  int                 mpiret;
  int                 level = 0;
  int                 maxlevel = 0;
  int                 helpme = 0;
  int                 parsed = 0;
  int                 threshold;
  int                 element_choice = 0;
  int                 invert_int = 0;
  bool                invert = false;
  png2mesh_image_t   *pngimage;
  sc_options_t       *opt;
  const char         *filename;
  const char         *help =
    "The program reads a png file and builds an adaptive mesh from it.\n"
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
  sc_options_add_string (opt, 'f', "file", &filename, "", "png file.");
  sc_options_add_switch (opt, 'i', "invert", &invert_int,
                         "Invert the refinement (refine bright areas, not dark).");
  sc_options_add_int (opt, 'l', "level", &level, 0,
                      "The initial refinement level of the mesh. Default 0.");
  sc_options_add_int (opt, 'm', "maxlevel", &maxlevel, 10,
                      "The maximum allowed refinement level of the mesh. Default 10.");
  sc_options_add_int (opt, 't', "threshold", &threshold, 100,
                      "How sensitive the refinement reacts to RGB values.\n"
                      "\t\t\t\t\tThe mesh is refined in areas with red + green + blue < threshold.\n"
                      "\t\t\t\t\tValues must be between 0 and 3*255.");
  sc_options_add_int (opt, 'e', "element_shape", &element_choice, 0,
                      "The shape of elements to use:\n"
                      "\t\t\t 0: quad\n"
                      "\t\t\t 1: triangle\n"
                      "\t\t\t 2: quad/triangle hybrid");

  parsed = sc_options_parse (-1, SC_LP_ERROR, opt, argc, argv);
  if (helpme) {
    /* display help message and usage */
    t8_global_productionf ("%s\n", help);
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else if (parsed >= 0 && 0 <= level && strcmp (filename, "") &&
           (element_choice >= 0 && element_choice <= 2)
           && level <= maxlevel && 0 <= threshold && threshold <= 3 * 255) {
    pngimage = png2mesh_read_png (filename);
    png2mesh_print_png (pngimage);
    #if 0
    png2mesh_adapt_context_t adapt_context;
    invert = invert_int != 0;
    if (pngimage != NULL) {
      adapt_context.image = pngimage;
      adapt_context.invert = invert;
      adapt_context.maxlevel = maxlevel;
      adapt_context.threshold = threshold;
      build_forest (level, element_choice, sc_MPI_COMM_WORLD, &adapt_context);
      png2mesh_image_cleanup (pngimage);
    }
    #endif
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
