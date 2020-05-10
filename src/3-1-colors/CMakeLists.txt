fips_begin_app(3-1-1-colors windowed)
    fips_vs_warning_level(3)
    fips_files(1-colors.c
    ../utility/camera.c)
    sokol_shader(shaders.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol hmm cimgui stb ui)
fips_end_app()
