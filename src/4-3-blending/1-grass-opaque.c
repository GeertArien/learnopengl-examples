//------------------------------------------------------------------------------
//  Blending (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "1-grass-opaque.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind_cube;
    sg_bindings bind_plane;
    sg_bindings bind_transparent;
    sg_pass_action pass_action;
    hmm_vec3 vegetation[5];
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    state.vegetation[0] = HMM_Vec3(-1.5f,  0.0f, -0.48f);
    state.vegetation[1] = HMM_Vec3( 1.5f,  0.0f,  0.51f);
    state.vegetation[2] = HMM_Vec3( 0.0f,  0.0f,  0.7f);
    state.vegetation[3] = HMM_Vec3(-0.3f,  0.0f, -2.3f);
    state.vegetation[4] = HMM_Vec3( 0.5f,  0.0f, -0.6f);

    float cube_vertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .content = cube_vertices,
        .label = "cube-vertices"
    });
    
    state.bind_cube.vertex_buffers[0] = cube_buffer;

    float plane_vertices[] = {
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f								
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(plane_vertices),
        .content = plane_vertices,
        .label = "plane-vertices"
    });

    state.bind_plane.vertex_buffers[0] = plane_buffer;

    float transparent_vertices[] = {
        // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

        0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    
    sg_buffer transparent_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(transparent_vertices),
        .content = transparent_vertices,
        .label = "transparent-vertices"
    });

    state.bind_transparent.vertex_buffers[0] = transparent_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader simple_shd = sg_make_shader(simple_shader_desc());

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = simple_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .label = "pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_image marble_img_id = sg_alloc_image();
    state.bind_cube.fs_images[SLOT_diffuse_texture] = marble_img_id;
    sg_image metal_img_id = sg_alloc_image();
    state.bind_plane.fs_images[SLOT_diffuse_texture] = metal_img_id;
    sg_image grass_img_id = sg_alloc_image();
    state.bind_transparent.fs_images[SLOT_diffuse_texture] = grass_img_id;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "metal.png",
            .img_id = metal_img_id,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "marble.jpg",
            .img_id = marble_img_id,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "grass.png",
            .img_id = grass_img_id,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };
    
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind_cube);

    vs_params.model = HMM_Translate(HMM_Vec3(-1.0f, 0.0f, -1.0f));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    vs_params.model = HMM_Translate(HMM_Vec3(2.0f, 0.0f, 0.0f));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    sg_apply_bindings(&state.bind_plane);

    vs_params.model = HMM_Mat4d(1.0f);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 6, 1);

    sg_apply_bindings(&state.bind_transparent);

    for(size_t i = 0; i < 5; i++) {
        vs_params.model = HMM_Translate(state.vegetation[i]);
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
        sg_draw(0, 6, 1);
    }

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    lopgl_handle_input(e);
}

void cleanup(void) {
    lopgl_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .gl_force_gles2 = true,
        .window_title = "Grass Opaque (LearnOpenGL)",
    };
}
