fips_begin_app(5-1-1-blinn-phong windowed)
    fips_vs_warning_level(3)
    fips_files(1-blinn-phong.c)
    sokol_shader(1-blinn-phong.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol)
fips_end_app()
