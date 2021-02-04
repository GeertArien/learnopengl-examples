//------------------------------------------------------------------------------
//  Point Shadows (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "1-omnidirectional-depth.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

static const int SHADOW_WIDTH = 1024;
static const int SHADOW_HEIGHT = 1024;

/* application state */
static struct {
    struct {
        sg_pass_action pass_action;
        sg_pass pass[6];
        sg_pipeline pip;
        sg_bindings bind;
    } depth;
    struct {
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } shadows;
    hmm_vec3 light_pos;
    hmm_mat4 light_space_matrix;
} state;

static void init(void) {
    lopgl_setup();

    state.light_pos = HMM_Vec3(0.f, 0.f, 0.f);

    /* create depth cubemap */
    sg_image_desc img_desc = {
        .type = SG_IMAGETYPE_CUBE,
        .render_target = true,
        .width = SHADOW_WIDTH,
        .height = SHADOW_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_w = SG_WRAP_CLAMP_TO_EDGE,
        .sample_count = 1,
        .label = "shadow-map-color-image"
    };
    sg_image color_img = sg_make_image(&img_desc);
    img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    img_desc.label = "shadow-map-depth-image";
    sg_image depth_img = sg_make_image(&img_desc);

    /* one pass for each cubemap face */
    for (size_t i = 0; i < 6; ++i) {
        state.depth.pass[i] = sg_make_pass(&(sg_pass_desc){
            .color_attachments[0] = { .image = color_img, .slice = i },
            .depth_stencil_attachment = {  .image = depth_img, .slice = i },
            .label = "shadow-map-pass"
        });
    }

    // sokol and webgl 1 do not support using the depth map as texture map
    // so instead we write the depth value to the color map
    state.shadows.bind.fs_images[SLOT_depth_map] = color_img;

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
    
    state.depth.bind.vertex_buffers[0] = cube_buffer;
    state.shadows.bind.vertex_buffers[0] = cube_buffer;

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
        .rasterizer = {
            .cull_mode = SG_CULLMODE_BACK,
            .face_winding = SG_FACEWINDING_CCW
        },
        .label = "shadows-pipeline"
    });

    state.depth.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={1.f, 1.f, 1.f, 1.0f} }
    };

    state.shadows.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void draw_room_cube(bool invert) {
    hmm_mat4 scale = HMM_Scale(HMM_MultiplyVec3f(HMM_Vec3(5.f, 5.f, 5.f), invert ? -1.f : 1.f));
    vs_params_t vs_params = {
        .model = scale 
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
}

void draw_cubes() {
    vs_params_t vs_params;
    hmm_mat4 translate = HMM_Translate(HMM_Vec3(4.f, -3.5f, 0.f));
    hmm_mat4 scale = HMM_Scale(HMM_Vec3(.5f, .5f, .5f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(2.f, 3.f, 1.f));
    scale = HMM_Scale(HMM_Vec3(.75f, .75f, .75f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(-3.f, -1.f, 0.f));
    scale = HMM_Scale(HMM_Vec3(.5f, .5f, .5f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(-1.5f, 1.f, 1.5f));
    scale = HMM_Scale(HMM_Vec3(.5f, .5f, .5f));
    vs_params.model = HMM_MultiplyMat4(translate, scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
    translate = HMM_Translate(HMM_Vec3(-1.5f, 2.f, -3.f));
    hmm_mat4 rotate = HMM_Rotate(60.f, HMM_NormalizeVec3(HMM_Vec3(1.f, 0.f, 1.f)));
    scale = HMM_Scale(HMM_Vec3(.75f, .75f, .75f));
    vs_params.model = HMM_MultiplyMat4(HMM_MultiplyMat4(translate, rotate), scale);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);
}

void frame(void) {
    lopgl_update();

    /* move light position over time */
    state.light_pos.Z = HMM_SinF((float)stm_sec(stm_now()) * .5f) * 3.f;

    /* create light space transform matrices */
    hmm_mat4 light_space_transforms[6];
    float near_plane = 1.0f;
    float far_plane  = 25.0f;
    hmm_mat4 shadow_proj = HMM_Perspective(90.0f, (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
    hmm_vec3 center = HMM_AddVec3(state.light_pos, HMM_Vec3(1.f, 0.f, 0.f));
    hmm_mat4 lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, -1.f, 0.f));
    light_space_transforms[SG_CUBEFACE_POS_X] = HMM_MultiplyMat4(shadow_proj, lookat);
    center = HMM_AddVec3(state.light_pos, HMM_Vec3(-1.f, 0.f, 0.f));
    lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, -1.f, 0.f));
    light_space_transforms[SG_CUBEFACE_NEG_X] = HMM_MultiplyMat4(shadow_proj, lookat);
    center = HMM_AddVec3(state.light_pos, HMM_Vec3(0.f, 1.f, 0.f));
    lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, 0.f, 1.f));
    light_space_transforms[SG_CUBEFACE_POS_Y] = HMM_MultiplyMat4(shadow_proj, lookat);
    center = HMM_AddVec3(state.light_pos, HMM_Vec3(0.f, -1.f, 0.f));
    lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, 0.f, -1.f));
    light_space_transforms[SG_CUBEFACE_NEG_Y] = HMM_MultiplyMat4(shadow_proj, lookat);
    center = HMM_AddVec3(state.light_pos, HMM_Vec3(0.f, 0.f, 1.f));
    lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, -1.f,  0.f));
    light_space_transforms[SG_CUBEFACE_POS_Z] = HMM_MultiplyMat4(shadow_proj, lookat);
    center = HMM_AddVec3(state.light_pos, HMM_Vec3(0.f, 0.f, -1.f));
    lookat = HMM_LookAt(state.light_pos, center, HMM_Vec3(0.f, -1.f,  0.f));
    light_space_transforms[SG_CUBEFACE_NEG_Z] = HMM_MultiplyMat4(shadow_proj, lookat);

    /* render depth of scene to cubemap (from light's perspective) */
    for (size_t i = 0; i < 6; ++i) {
        sg_begin_pass(state.depth.pass[i], &state.depth.pass_action);
        sg_apply_pipeline(state.depth.pip);
        sg_apply_bindings(&state.depth.bind);

        vs_params_depth_t vs_params_depth = {
            .light_space_matrix = light_space_transforms[i]
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_depth, &vs_params_depth, sizeof(vs_params_depth));

        fs_params_depth_t fs_params_depth = {
            .light_pos = state.light_pos,
            .far_plane = far_plane
        };

        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_depth, &fs_params_depth, sizeof(fs_params_depth));

        draw_cubes();
        // don't inver the room cube for computing the depth map
        draw_room_cube(false);
        sg_end_pass();
    }

    /* render scene as normal using the generated depth/shadow map */
    sg_begin_default_pass(&state.shadows.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.shadows.pip);
    sg_apply_bindings(&state.shadows.bind);

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_shadows_t vs_params_shadows = {
        .projection = projection,
        .view = view,
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &vs_params_shadows, sizeof(vs_params_shadows));

    fs_params_shadows_t fs_params_shadows = {
        .light_pos = state.light_pos,
        .far_plane = far_plane
    };

    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params_shadows, &fs_params_shadows, sizeof(fs_params_shadows));

    draw_cubes();

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_shadows, &vs_params_shadows, sizeof(vs_params_shadows));

    // invert the room cube so we can inside the cube
    draw_room_cube(true);

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
        .window_title = "Omnidirectional Depth (LearnOpenGL)",
    };
}
