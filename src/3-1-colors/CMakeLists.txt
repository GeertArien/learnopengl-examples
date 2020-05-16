fips_begin_app(3-1-1-scene windowed)
    fips_vs_warning_level(3)
    fips_files(
        1-scene.c
        ../utility/camera.c
    )
    sokol_shader(shaders.glsl ${slang})
    fips_deps(sokol hmm cimgui ui)
fips_end_app()
