# we're using texelFetch in the shader which is not supported in glsl100
if(slang STREQUAL "glsl300es:glsl100")
    set(slang "glsl300es")
endif()

fips_begin_app(4-9-1-lines windowed)
    fips_vs_warning_level(3)
    fips_files(1-lines.c)
    sokol_shader(1-lines.glsl ${slang})
    fips_deps(sokol)
fips_end_app()
