//------------------------------------------------------------------------------
//  Advanced GLSL (4)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "4-uniform-buffers.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip_red;
    sg_pipeline pip_green;
    sg_pipeline pip_blue;
    sg_pipeline pip_yellow;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    lopgl_setup();

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f,  0.5f,
        0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
        0.5f,  0.5f, -0.5f,
        0.5f,  0.5f,  0.5f,
        0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };
    
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "vertices-cube"
    });

    /* create shaders from code-generated sg_shader_desc */
    sg_shader shd_red = sg_make_shader(red_shader_desc());
    sg_shader shd_green = sg_make_shader(green_shader_desc());
    sg_shader shd_blue = sg_make_shader(blue_shader_desc());
    sg_shader shd_yellow = sg_make_shader(yellow_shader_desc());

    sg_pipeline_desc pip_desc = {
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .label = "cube-pipeline"
    };

    /* create a pipeline objects */
    pip_desc.shader = shd_red;
    state.pip_red = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_green;
    state.pip_green = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_blue;
    state.pip_blue = sg_make_pipeline(&pip_desc);
    pip_desc.shader = shd_yellow;
    state.pip_yellow = sg_make_pipeline(&pip_desc);

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    sg_apply_pipeline(state.pip_red);
    sg_apply_bindings(&state.bind);

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_view_projection_t vs_vp = {
        .view = view,
        .projection = projection
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &vs_vp, sizeof(vs_vp));

    vs_model_t vs_m = {
        .model = HMM_Translate(HMM_Vec3(-0.75f, 0.75f, 0.0f))       // move top-left
    };
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &vs_m, sizeof(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_green);
    sg_apply_bindings(&state.bind);
    // we need to re-apply the uniforms after applying a new pipeline, sort of defeats the purpose of this example...
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &vs_vp, sizeof(vs_vp));
    vs_m.model = HMM_Translate(HMM_Vec3(0.75f, 0.75f, 0.0f));       // move top-right
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &vs_m, sizeof(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_yellow);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &vs_vp, sizeof(vs_vp));
    vs_m.model = HMM_Translate(HMM_Vec3(-0.75f, -0.75f, 0.0f));     // move bottom-left
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &vs_m, sizeof(vs_m));
    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_blue);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_view_projection, &vs_vp, sizeof(vs_vp));
    vs_m.model = HMM_Translate(HMM_Vec3(0.75f, -0.75f, 0.0f));      // move bottom-right
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_model, &vs_m, sizeof(vs_m));
    sg_draw(0, 36, 1);

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
        .window_title = "Uniform Buffers (LearnOpenGL)",
    };
}
