//------------------------------------------------------------------------------
//  Anti Aliasing (3)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "3-grayscale-msaa.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    struct {
        sg_pass pass;
        sg_pass_desc pass_desc;
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } offscreen;
    struct { 
        sg_pass_action pass_action;
        sg_pipeline pip;
        sg_bindings bind;
    } display;
} state;

/* called initially and when window size changes */
void create_offscreen_pass(int width, int height) {
    /* destroy previous resource (can be called for invalid id) */
    sg_destroy_pass(state.offscreen.pass);
    sg_destroy_image(state.offscreen.pass_desc.color_attachments[0].image);
    sg_destroy_image(state.offscreen.pass_desc.depth_stencil_attachment.image);

    /* create offscreen rendertarget images and pass */
    sg_image_desc color_img_desc = {
        .render_target = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        /* we need to set the sample count in both the rendertarget and the pipeline */
        .sample_count = 4,
        /* Webgl 1.0 does not support repeat for textures that are not a power of two in size */
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .label = "color-image"
    };
    sg_image color_img = sg_make_image(&color_img_desc);

    sg_image_desc depth_img_desc = color_img_desc;
    depth_img_desc.pixel_format = SG_PIXELFORMAT_DEPTH;
    depth_img_desc.label = "depth-image";
    sg_image depth_img = sg_make_image(&depth_img_desc);

    state.offscreen.pass_desc = (sg_pass_desc){
        .color_attachments[0].image = color_img,
        .depth_stencil_attachment.image = depth_img,
        .label = "offscreen-pass"
    };
    state.offscreen.pass = sg_make_pass(&state.offscreen.pass_desc);

    /* also need to update the fullscreen-quad texture bindings */
    state.display.bind.fs_images[SLOT_diffuse_texture] = color_img;
}

static void init(void) {
    lopgl_setup();

    /* a render pass with one color- and one depth-attachment image */
    create_offscreen_pass(sapp_width(), sapp_height());

    /* a pass action to clear offscreen framebuffer */
    state.offscreen.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    /* a pass action for rendering the fullscreen-quad */
    state.display.pass_action = (sg_pass_action) {
        .colors[0].action=SG_ACTION_DONTCARE,
        .depth.action=SG_ACTION_DONTCARE,
        .stencil.action=SG_ACTION_DONTCARE
    };
    
    float cube_vertices[] = {
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

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .content = cube_vertices,
        .label = "cube-vertices"
    });

    float quad_vertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    sg_buffer quad_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .content = quad_vertices,
        .label = "quad-vertices"
    });
    
    state.offscreen.bind.vertex_buffers[0] = cube_buffer;

    /* resource bindings to render an fullscreen-quad */
    state.display.bind.vertex_buffers[0] = quad_buffer;

    /* create a pipeline object for offscreen pass */
    state.offscreen.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(offscreen_shader_desc()),
        .layout = {
            .attrs = {
                [ATTR_vs_offscreen_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .blend = {
            .color_format = SG_PIXELFORMAT_RGBA8,
            .depth_format = SG_PIXELFORMAT_DEPTH
        },
        /* we need to set the sample count in both the rendertarget and the pipeline */
        .rasterizer.sample_count = 4,
        .label = "offscreen-pipeline"
    });

    /* and another pipeline-state-object for the display pass */
    state.display.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs = {
                [ATTR_vs_display_a_pos].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_display_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .shader = sg_make_shader(display_shader_desc()),
        .label = "display-pipeline"
    });
}

void frame(void) {
    lopgl_update();

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    /* the offscreen pass, rendering an rotating, untextured cube into a render target image */
    sg_begin_pass(state.offscreen.pass, &state.offscreen.pass_action);
    sg_apply_pipeline(state.offscreen.pip);
    sg_apply_bindings(&state.offscreen.bind);

    vs_params.model = HMM_Translate(HMM_Vec3(0.f, 0.f, 0.f));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    sg_end_pass();

    /* and the display-pass, rendering a quad, using the previously rendered 
       offscreen render-target as texture */
    sg_begin_default_pass(&state.display.pass_action, sapp_width(), sapp_height());
    sg_apply_pipeline(state.display.pip);
    sg_apply_bindings(&state.display.bind);
    sg_draw(0, 6, 1);

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}



void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_RESIZED) {
        create_offscreen_pass(e->framebuffer_width, e->framebuffer_height);
    }

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
        .window_title = "Grayscale MSAA (LearnOpenGL)",
    };
}
