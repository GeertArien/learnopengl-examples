fips_begin_app(1-9-1-lookat windowed)
    fips_vs_warning_level(3)
    fips_files(1-lookat.c)
    sokol_shader(shaders.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol)
fips_end_app()

fips_begin_app(1-9-2-walk windowed)
    fips_vs_warning_level(3)
    fips_files(2-walk.c)
    sokol_shader(shaders.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol)
fips_end_app()

fips_begin_app(1-9-3-look windowed)
    fips_vs_warning_level(3)
    fips_files(3-look.c)
    sokol_shader(shaders.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol)
fips_end_app()
