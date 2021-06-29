# Copy all .png files from examples folder to the install folder.

# Find all .png files in examples/ dir
file(GLOB files "./*.png")
foreach(file ${files})
  # Remove path from filename and store in variable
  get_filename_component(file_basename ${file} NAME)
  # Copy original file to new examples/ directory
  configure_file(${file} ${file_basename} COPYONLY)
endforeach()
