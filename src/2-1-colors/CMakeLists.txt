fips_begin_app(2-1-1-scene windowed)
    fips_vs_warning_level(3)
    fips_files(
        1-scene.c
        ../utility/fp_camera.c
        ../utility/orbital_camera.c
        ../utility/camera.c
    )
    sokol_shader(shaders.glsl ${slang})
    fips_deps(sokol hmm)
fips_end_app()
