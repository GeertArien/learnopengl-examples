fips_begin_app(2-4-1-diffuse-map windowed)
    fips_vs_warning_level(3)
    fips_files(
        1-diffuse-map.c
        ../utility/camera.c
    )
    sokol_shader(1-diffuse-map.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol hmm stb)
fips_end_app()

fips_begin_app(2-4-2-specular-map windowed)
    fips_vs_warning_level(3)
    fips_files(
        2-specular-map.c
        ../utility/camera.c
    )
    sokol_shader(2-specular-map.glsl ${slang})
    fipsutil_copy(textures-assets.yml)
    fips_deps(sokol hmm stb)
fips_end_app()
