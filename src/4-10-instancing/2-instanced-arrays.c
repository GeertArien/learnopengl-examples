//------------------------------------------------------------------------------
//  Instancing (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "../libs/hmm/HandmadeMath.h"
#include "2-instanced-arrays.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "string.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    lopgl_setup();
    
    if (sapp_gles2()) {
        /* this demo needs GLES3/WebGL because we are using gl_InstanceID in the shader */
        return;
    }

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc());

    float vertices[] = {
        // positions     // colors
        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
        -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,   
         0.05f,  0.05f,  0.0f, 1.0f, 1.0f		    		
    };  

    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "quad-vertices"
    });

    hmm_vec2 translations[100];
    int index = 0;
    float offset = 0.1f;
    for(int y = -10; y < 10; y += 2) {
        for(int x = -10; x < 10; x += 2) {
            float x_pos = (float)x / 10.0f + offset;
            float y_pos = (float)y / 10.0f + offset;
            translations[index++] = HMM_Vec2(x_pos, y_pos);
        }
    }

    state.bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(translations),
        .content = translations,
        .label = "offsets"
    });

    /* a pipeline state object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            /* vertex buffer at slot 1 must step per instance */
            .buffers[1].step_func = SG_VERTEXSTEP_PER_INSTANCE,
            .attrs = {
                [ATTR_vs_aPos]      = { .format=SG_VERTEXFORMAT_FLOAT2, .buffer_index=0 },
                [ATTR_vs_aColor]    = { .format=SG_VERTEXFORMAT_FLOAT3, .buffer_index=0 },
                [ATTR_vs_aOffset]   = { .format=SG_VERTEXFORMAT_FLOAT2, .buffer_index=1 }
            }
        },
        .label = "quad-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    /* can't do anything useful on GLES2/WebGL */
    if (sapp_gles2()) {
        lopgl_render_gles2_fallback();
        return;
    }

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 6, 100);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .window_title = "Instanced Arrays (LearnOpenGL)",
    };
}
