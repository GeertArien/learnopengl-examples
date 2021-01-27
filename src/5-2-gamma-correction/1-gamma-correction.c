//------------------------------------------------------------------------------
//  Gamma Correction (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "1-gamma-correction.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    hmm_vec4 light_positions[4];
    hmm_vec4 light_colors[4];
    bool gamma;
    uint8_t file_buffer[2 * 1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void init(void) {
    lopgl_setup();

    // default is gamma enabled
    state.gamma = true;

    // set light positions and colors
    state.light_positions[0] = HMM_Vec4(-3.0f, 0.0f, 0.0f, 1.0f);
    state.light_positions[1] = HMM_Vec4(-1.0f, 0.0f, 0.0f, 1.0f);
    state.light_positions[2] = HMM_Vec4(1.0f, 0.0f, 0.0f, 1.0f);
    state.light_positions[3] = HMM_Vec4(3.0f, 0.0f, 0.0f, 1.0f);

    state.light_colors[0] = HMM_Vec4(0.25f, 0.25f, 0.25f, 1.0f);
    state.light_colors[1] = HMM_Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    state.light_colors[2] = HMM_Vec4(0.75f, 0.75f, 0.75f, 1.0f);
    state.light_colors[3] = HMM_Vec4(1.f, 1.f, 1.f, 1.0f);

    float vertices[] = {
        // positions            // normals         // texcoords
         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };

    sg_buffer plane_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "plane-vertices"
    });
    
    state.bind.vertex_buffers[0] = plane_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(blinn_phong_shader_desc());

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .label = "object-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_image img_id_floor = sg_alloc_image();
    state.bind.fs_images[SLOT_floor_texture] = img_id_floor;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "wood.png",
            .img_id = img_id_floor,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

static void render_ui() {
    sdtx_canvas(sapp_width()*0.5f, sapp_height()*0.5f);
    sdtx_origin(sapp_width()*0.5f/8.f - 25.f, 0.25f);       // each character occupies a grid fo 8x8
    sdtx_home();

    sdtx_color4b(0xff, 0x00, 0x00, 0xaf);
    sdtx_printf("%sGamma Correction\n", state.gamma ? "": "No ");
    sdtx_puts("Toggle:\t\t'SPACE'");
    sdtx_draw();
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

    fs_params_t fs_params = {
        .light_pos[0] = state.light_positions[0],
        .light_colors[0] = state.light_colors[0],
        .light_pos[1] = state.light_positions[1],
        .light_colors[1] = state.light_colors[1],
        .light_pos[2] = state.light_positions[2],
        .light_colors[2] = state.light_colors[2],
        .light_pos[3] = state.light_positions[3],
        .light_colors[3] = state.light_colors[3],
        .view_pos = lopgl_camera_position(),
        .gamma = state.gamma ? 1.f : 0.f 
    };

    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));

    sg_draw(0, 6, 1);

    lopgl_render_help();

    if (lopgl_ui_visible()) {
        render_ui();
    }

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    lopgl_handle_input(e);

    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_SPACE) {
            state.gamma = !state.gamma;
        }
    }
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
        .window_title = "Gamma Correction (LearnOpenGL)",
    };
}
