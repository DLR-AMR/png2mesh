# Add executable called "png2mesh_test_read" that is built from the source files
# "png2mesh_test_read.c". The extensions are automatically found.
add_executable (png2mesh_test_read png2mesh_test_read.c)

# Link the executable to the png2mesh library. Since the png2mesh library has
# public include directories we will use those link directories when building
# tests.
target_link_libraries (png2mesh_test_read LINK_PUBLIC png2mesh)
