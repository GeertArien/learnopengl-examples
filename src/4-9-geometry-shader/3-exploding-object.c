//------------------------------------------------------------------------------
//  Geometry Shader (3)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "3-exploding-object.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "fast_obj/lopgl_fast_obj.h"

static const char* filename = "backpack.obj";

typedef struct mesh_t {
    sg_pipeline pip;
    sg_bindings bind;
    unsigned int face_count;
} mesh_t;

/* application state */
static struct {
    mesh_t mesh; 
    sg_pass_action pass_action;
    uint8_t file_buffer[16 * 1024 * 1024];
    float vertex_buffer[70000 * 3 * 5];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void load_obj_callback(lopgl_obj_response_t* response) {
    fastObjMesh* mesh = response->mesh;
    state.mesh.face_count = mesh->face_count;

    for (unsigned int i = 0; i < mesh->face_count * 3; ++i) {
        fastObjIndex vertex = mesh->indices[i];

        unsigned int pos = i * 5;
        unsigned int v_pos = vertex.p * 3;
        unsigned int t_pos = vertex.t * 2;

        memcpy(state.vertex_buffer + pos, mesh->positions + v_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 3, mesh->texcoords + t_pos, 2 * sizeof(float));
    }

    state.mesh.bind.vs_images[SLOT_vertex_texture] = sg_make_image(&(sg_image_desc){
        .width = 1024,
        .height = mesh->face_count*3*5/1024 + 1,
        .pixel_format = SG_PIXELFORMAT_R32F,
        /* set filter to nearest, webgl2 does not support filtering for float textures */
        .mag_filter = SG_FILTER_NEAREST,
        .min_filter = SG_FILTER_NEAREST,
        .content.subimage[0][0] = {
            .ptr = state.vertex_buffer,
            .size = sizeof(state.vertex_buffer)
        },
        .label = "color-texture"
    });

    sg_image img_id = sg_alloc_image();
    state.mesh.bind.fs_images[SLOT_diffuse_texture] = img_id;

    lopgl_load_image(&(lopgl_image_request_t){
        .path = mesh->materials[0].map_Kd.name,
        .img_id = img_id,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });
}

static void init(void) {
    lopgl_setup();

    if (sapp_gles2()) {
        /* this demo needs GLES3/WebGL because we are using texelFetch in the shader */
        return;
    }

    /* create shader from code-generated sg_shader_desc */
    sg_shader phong_shd = sg_make_shader(phong_shader_desc());

    /* create a pipeline object for object */
    state.mesh.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = phong_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_dummy].format = SG_VERTEXFORMAT_FLOAT,
            },
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "object-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    lopgl_load_obj(&(lopgl_obj_request_t){
        .path = filename,
        .callback = load_obj_callback,
        .fail_callback = fail_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
    });
}

void frame(void) {
    /* can't do anything useful on GLES2/WebGL */
    if (sapp_gles2()) {
        lopgl_render_gles2_fallback();
        return;
    }

    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    if (state.mesh.face_count > 0) {
        hmm_mat4 view = lopgl_view_matrix();
        hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

        vs_params_t vs_params = {
            .model = HMM_Mat4d(1.f),
            .view = view,
            .projection = projection,
            .time = (float)stm_sec(stm_now())
        };

        sg_apply_pipeline(state.mesh.pip);
        sg_apply_bindings(&state.mesh.bind);

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

        sg_draw(0, state.mesh.face_count * 3, 1);
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
        .window_title = "Exploding Object (LearnOpenGL)",
    };
}
