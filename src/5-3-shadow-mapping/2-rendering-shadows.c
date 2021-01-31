//------------------------------------------------------------------------------
//  Shadow Mapping (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "2-rendering-shadows.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    struct {
        sg_pass_action pass_action;
        sg_pass pass;
        sg_pipeline pip;
        sg_bindings bind_cube;
        sg_bindings bind_plane;
    } depth;
    struct {
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind_cube;
        sg_bindings bind_plane;
    } shadows;
    hmm_vec3 light_pos;
    hmm_mat4 light_space_matrix;
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    // compute light space matrix
    state.light_pos = HMM_Vec3(-2.f, 4.f, -1.f);
    float near_plane = 1.f;
    float far_plane = 7.5f;
    hmm_mat4 light_projection = HMM_Orthographic(-10.f, 10.f, -10.f, 10.f, near_plane, far_plane);
    hmm_mat4 light_view = HMM_LookAt(state.light_pos, HMM_Vec3(0.f, 0.f, 0.f), HMM_Vec3(0.f, 1.f, 0.f));
    state.light_space_matrix = HMM_MultiplyMat4(light_projection, light_view);

     /* a render pass with one color- and one depth-attachment image */
    sg_image_desc img_desc = {
        .render_target = true,
        .width = 1024,
        .height = 1024,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .sample_count = 1,
        .label = "shadow-map-color-image"
    };
    sg_image color_img = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "shadow-map-depth-image";
    sg_image depth_img = sg_make_image(&img_desc);
    state.depth.pass = sg_make_pass(&(sg_pass_desc){
        .color_attachments[0].image = color_img,
        .depth_stencil_attachment.image = depth_img,
        .label = "shadow-map-pass"
    });

    // sokol and webgl 1 do not support using the depth map as texture map
    // so instead we write the depth value to the color map
    state.shadows.bind_cube.fs_images[SLOT_shadow_map] = color_img;
    state.shadows.bind_plane.fs_images[SLOT_shadow_map] = color_img;

    float cube_vertices[] = {
        // back face
        -1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 0.f, // bottom-left
         1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 1.f, // top-right
         1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 0.f, // bottom-right         
         1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 1.f, 1.f, // top-right
        -1.f, -1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 0.f, // bottom-left
        -1.f,  1.f, -1.f,  0.f,  0.f, -1.f, 0.f, 1.f, // top-left
        // front face
        -1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 0.f, // bottom-left
         1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 0.f, // bottom-right
         1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 1.f, // top-right
         1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 1.f, 1.f, // top-right
        -1.f,  1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 1.f, // top-left
        -1.f, -1.f,  1.f,  0.f,  0.f,  1.f, 0.f, 0.f, // bottom-left
        // left face
        -1.f,  1.f,  1.f, -1.f,  0.f,  0.f, 1.f, 0.f, // top-right
        -1.f,  1.f, -1.f, -1.f,  0.f,  0.f, 1.f, 1.f, // top-left
        -1.f, -1.f, -1.f, -1.f,  0.f,  0.f, 0.f, 1.f, // bottom-left
        -1.f, -1.f, -1.f, -1.f,  0.f,  0.f, 0.f, 1.f, // bottom-left
        -1.f, -1.f,  1.f, -1.f,  0.f,  0.f, 0.f, 0.f, // bottom-right
        -1.f,  1.f,  1.f, -1.f,  0.f,  0.f, 1.f, 0.f, // top-right
        // right face
         1.f,  1.f,  1.f,  1.f,  0.f,  0.f, 1.f, 0.f, // top-left
         1.f, -1.f, -1.f,  1.f,  0.f,  0.f, 0.f, 1.f, // bottom-right
         1.f,  1.f, -1.f,  1.f,  0.f,  0.f, 1.f, 1.f, // top-right         
         1.f, -1.f, -1.f,  1.f,  0.f,  0.f, 0.f, 1.f, // bottom-right
         1.f,  1.f,  1.f,  1.f,  0.f,  0.f, 1.f, 0.f, // top-left
         1.f, -1.f,  1.f,  1.f,  0.f,  0.f, 0.f, 0.f, // bottom-left     
        // bottom face
        -1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 0.f, 1.f, // top-right
         1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 1.f, 1.f, // top-left
         1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 1.f, 0.f, // bottom-left
         1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 1.f, 0.f, // bottom-left
        -1.f, -1.f,  1.f,  0.f, -1.f,  0.f, 0.f, 0.f, // bottom-right
        -1.f, -1.f, -1.f,  0.f, -1.f,  0.f, 0.f, 1.f, // top-right
        // top face
        -1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 0.f, 1.f, // top-left
         1.f,  1.f , 1.f,  0.f,  1.f,  0.f, 1.f, 0.f, // bottom-right
         1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 1.f, 1.f, // top-right     
         1.f,  1.f,  1.f,  0.f,  1.f,  0.f, 1.f, 0.f, // bottom-right
        -1.f,  1.f, -1.f,  0.f,  1.f,  0.f, 0.f, 1.f, // top-left
        -1.f,  1.f,  1.f,  0.f,  1.f,  0.f, 0.f, 0.f  // bottom-left        
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .content = cube_vertices,
        .label = "cube-vertices"
    });
    
    state.depth.bind_cube.vertex_buffers[0] = cube_buffer;
    state.shadows.bind_cube.vertex_buffers[0] = cube_buffer;

    float plane_vertices[] = {
        // positions         // normals      // texcoords
         25.f, -.5f,  25.f,  0.f, 1.f, 0.f,  25.f,  0.f,
        -25.f, -.5f,  25.f,  0.f, 1.f, 0.f,   0.f,  0.f,
        -25.f, -.5f, -25.f,  0.f, 1.f, 0.f,   0.f, 25.f,

         25.f, -.5f,  25.f,  0.f, 1.f, 0.f,  25.f,  0.f,
        -25.f, -.5f, -25.f,  0.f, 1.f, 0.f,   0.f, 25.f,
         25.f, -.5f, -25.f,  0.f, 1.f, 0.f,  25.f, 25.f
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(plane_vertices),
        .content = plane_vertices,
        .label = "plane-vertices"
    });
    
    state.depth.bind_plane.vertex_buffers[0] = plane_buffer;
    state.shadows.bind_plane.vertex_buffers[0] = plane_buffer;

    sg_shader shd_depth = sg_make_shader(depth_shader_desc());

    state.depth.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_depth,
        .layout = {
            /* Buffer's normal and texture coords are skipped */
            .buffers[0].stride = 8 * sizeof(float),
            .attrs = {
                [ATTR_vs_depth_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .blend = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH
        },
        .label = "depth-pipeline"
    });

    sg_shader shd_shadows = sg_make_shader(shadows_shader_desc());

    state.shadows.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd_shadows,
        .layout = {
            .attrs = {
                [ATTR_vs_shadows_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_shadows_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_shadows_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "shadows-pipeline"
    });

    state.depth.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={1.f, 1.f, 1.f, 1.0f} }
    };

    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_image img_id_diffuse = sg_alloc_image();
    state.shadows.bind_cube.fs_images[SLOT_diffuse_texture] = img_id_diffuse;
    state.shadows.bind_plane.fs_images[SLOT_diffuse_texture] = img_id_diffuse;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "wood.png",
            .img_id = img_id_diffuse,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void draw_cubes() {
    vs_params_t vs_params = {
        .light_space_matrix = state.light_space_matrix,
        .model = HMM_Mat4d(1.f)
    };

    hmm_mat4 translate = HMM_Translate(HMM_Vec3(0.f, 1.5f, 0.f));
    hmm_mat4 scale = HMM_Scale(HMM_Vec3(.5f, .5f, .5f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(2.f, 0.f, 1.f));
    scale = HMM_Scale(HMM_Vec3(.5f, .5f, .5f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(-1.f, 0.f, 2.f));
    hmm_mat4 rotate = HMM_Rotate(60.f, HMM_NormalizeVec3(HMM_Vec3(1.f, 0.f, 1.f)));
    scale = HMM_Scale(HMM_Vec3(.25f, .25f, .25f));
    vs_params.model = HMM_MultiplyMat4(HMM_MultiplyMat4(translate, rotate), scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
}

void frame(void) {
    lopgl_update();

    /* 1. render depth of scene to texture (from light's perspective) */
    sg_begin_pass(state.depth.pass, &state.depth.pass_action);
    sg_apply_pipeline(state.depth.pip);

    /* plane */
    sg_apply_bindings(&state.depth.bind_plane);

    vs_params_t vs_params = {
        .light_space_matrix = state.light_space_matrix,
        .model = HMM_Mat4d(1.f)
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 6, 1);

    /* cubes */
    sg_apply_bindings(&state.depth.bind_cube);
    draw_cubes();
    sg_end_pass();

    /* 2. render scene as normal using the generated depth/shadow map */
    sg_begin_default_pass(&state.shadows.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.shadows.pip);

    /* plane */
    sg_apply_bindings(&state.shadows.bind_plane);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_shadows_t vs_params_shadows = {
        .projection = projection,
        .view = view
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &vs_params_shadows, sizeof(vs_params_shadows));

    fs_params_shadows_t fs_params_shadows = {
        .light_pos = state.light_pos,
        .view_pos = lopgl_camera_position(),
    };

    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_shadows, &fs_params_shadows, sizeof(fs_params_shadows));

    sg_draw(0, 6, 1);

    /* cubes */
    sg_apply_bindings(&state.shadows.bind_cube);
    draw_cubes();

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
        .window_title = "Rendering Shadows (LearnOpenGL)",
    };
}
