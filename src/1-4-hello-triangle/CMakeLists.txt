fips_begin_app(1-4-1-triangle windowed)
    fips_vs_warning_level(3)
    fips_files(1-triangle.c)
    sokol_shader(1-triangle.glsl ${slang})
    fips_deps(sokol)
fips_end_app()

fips_begin_app(1-4-2-quad windowed)
    fips_vs_warning_level(3)
    fips_files(2-quad.c)
    sokol_shader(2-quad.glsl ${slang})
    fips_deps(sokol)
fips_end_app()

fips_begin_app(1-4-3-quad-wireframe windowed)
    fips_vs_warning_level(3)
    fips_files(3-quad-wireframe.c)
    sokol_shader(3-quad-wireframe.glsl ${slang})
    fips_deps(sokol)
fips_end_app()
