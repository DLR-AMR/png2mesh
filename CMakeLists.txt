#
# This file was copied and modified from https://cmake.org/examples/
#

# CMakeLists files in this project can
# refer to the root source directory of the project as ${PNG2MESH_SOURCE_DIR} and
# to the root binary directory of the project as ${PNG2MESH_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (PNG2MESH)

# Set compilers to mpicc and mpicxx
set(CMAKE_C_COMPILER mpicc)
set(CMAKE_CXX_COMPILER mpicxx)

## Add t8code include directories
#INCLUDE_DIRECTORIES($ENV{T8_INCLUDE})
#INCLUDE_DIRECTORIES($ENV{T8_INCLUDE}/t8_cmesh)
## Add t8code library directories
#LINK_DIRECTORIES($ENV{T8_LIB})

# Searchers for t8code build with CMake
# To add a seach path use 
#  -DT8CODE_DIR=PATH,
#  -DP4EST_DIR=PATH,
#  -DSC_DIR=PATH,
find_package ( T8CODE REQUIRED)

# Using PNG_INCLUDE and PNG_LIB is only necessary when
# the compiler does not find the PNG library.
# Add png include directories
INCLUDE_DIRECTORIES($ENV{PNG_INCLUDE})
# Add png library directories
LINK_DIRECTORIES($ENV{PNG_LIB})


# Create a library called "png2mesh" which includes the source files.
# The extension is already found. Any number of sources could be listed here.
add_library (png2mesh png2mesh_build_mesh.cxx png2mesh_readpng.c)

# Link library against t8code, p4est, sc, and png
target_link_libraries (png2mesh PRIVATE T8CODE::T8 )
target_link_libraries (png2mesh png )

# Make sure the compiler can find include files for our png2mesh library
# when other libraries or executables link to png2mesh
target_include_directories (png2mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add executable called "png2mesh_demo" that is built from the source file
# "png2mesh_build_mesh.cxx". The extensions are automatically found.
add_executable (png2mesh_demo png2mesh_build_mesh.cxx)

# Link the executable to the png2mesh library. Since the library has
# public include directories we will use those link directories when building
# png2mesh_demo.
target_link_libraries (png2mesh_demo LINK_PUBLIC png2mesh)

# Recurse into the "test" subdirectory. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (test)

# Recurse into "examples" subdirectory.
add_subdirectory (examples)
