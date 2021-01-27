//------------------------------------------------------------------------------
//  Geometry Shader (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "2-houses.glsl.h"
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
        /* this demo needs GLES3/WebGL because we are using texelFetch in the shader */
        return;
    }

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(simple_shader_desc());

    /* a pipeline state object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                /* dummy vertex attribute, otherwise sokol complains */
                [ATTR_vs_a_dummy].format = SG_VERTEXFORMAT_FLOAT,
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .label = "vertices-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    float positions[] = {
        -0.5f, 0.5f, // top-left
        0.5f,  0.5f, // top-right
        0.5f,  -0.5f, // bottom-right
        -0.5f, -0.5f  // bottom-left
    };

    state.bind.vs_images[SLOT_position_texture] = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RG32F,
        /* set filter to nearest, webgl2 does not support filtering for float textures */
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = SG_FILTER_NEAREST,
        .content.subimage[0][0] = {
            .ptr = positions,
            .size = sizeof(positions)
        },
        .label = "positions-texture"
    });

    float colors[] = {
        1.0f, 0.0f, 0.0f, 1.0,  // top-left
        0.0f, 1.0f, 0.0f, 1.0,  // top-right
        0.0f, 0.0f, 1.0f, 1.0,  // bottom-right
        1.0f, 1.0f, 0.0f, 1.0   // bottom-left
    };

    state.bind.vs_images[SLOT_color_texture] = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_RGBA32F,
        /* set filter to nearest, webgl2 does not support filtering for float textures */
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = SG_FILTER_NEAREST,
        .content.subimage[0][0] = {
            .ptr = colors,
            .size = sizeof(colors)
        },
        .label = "color-texture"
    });
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
    sg_draw(0, 4*9, 1);
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
        .window_title = "Houses (LearnOpenGL)",
    };
}
