# select the 3D backend for sokol_gfx.h and sokol_app.h
# the sokol_config() macro is in the top-level CMakeLists.txt file!
sokol_config()

fips_ide_group(Src/2-3-hello-window)
add_subdirectory(2-3-hello-window)
fips_ide_group(Src/2-4-hello-triangle)
add_subdirectory(2-4-hello-triangle)
fips_ide_group(Src/2-5-shaders)
add_subdirectory(2-5-shaders)
fips_ide_group(Src/2-6-textures)
add_subdirectory(2-6-textures)
fips_ide_group(Src/2-7-transformations)
add_subdirectory(2-7-transformations)
fips_ide_group(Src/2-8-coordinate-systems)
add_subdirectory(2-8-coordinate-systems)
fips_ide_group(Src/2-9-camera)
add_subdirectory(2-9-camera)
