fips_begin_app(3-4-1-diffuse-map windowed)
    fips_vs_warning_level(3)
    fips_files(
        1-diffuse-map.c
        ../utility/camera.c
    )
    sokol_shader(1-diffuse-map.glsl ${slang})
    fips_deps(sokol hmm cimgui ui)
fips_end_app()
