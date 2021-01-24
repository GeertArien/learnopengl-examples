//------------------------------------------------------------------------------
//  Cubemaps (5)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "5-refraction-backpack.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

typedef struct mesh_t {
    sg_pipeline pip;
    sg_bindings bind;
    unsigned int face_count;
} mesh_t;

/* application state */
static struct {
    mesh_t mesh; 
    sg_pipeline pip_skybox;
    sg_bindings bind_skybox;
    sg_pass_action pass_action;
    float vertex_buffer[70000 * 3 * 6];
    uint8_t file_buffer[8 * 1024 * 1024];
    uint8_t cubemap_buffer[6 * 1024 * 1024];
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

        unsigned int pos = i * 6;
        unsigned int v_pos = vertex.p * 3;
        unsigned int n_pos = vertex.n * 3;

        memcpy(state.vertex_buffer + pos, mesh->positions + v_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 3, mesh->normals + n_pos, 3 * sizeof(float));
    }

    sg_buffer mesh_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = mesh->face_count * 3 * 6 * sizeof(float),
        .content = state.vertex_buffer,
        .label = "backpack-vertices"
    });
    
    state.mesh.bind.vertex_buffers[0] = mesh_buffer;
}


static void init(void) {
    lopgl_setup();

    float skybox_vertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    sg_buffer skybox_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(skybox_vertices),
        .content = skybox_vertices,
        .label = "skybox-vertices"
    });
    
    state.bind_skybox.vertex_buffers[0] = skybox_buffer;

    /* create a pipeline object for the cube */
    state.mesh.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(simple_shader_desc()),
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_normal].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .label = "mesh-pipeline"
    });

    /* create a pipeline object for the skybox */
    state.pip_skybox = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(skybox_shader_desc()),
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
        },
        .label = "skybox-pipeline"
    });

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_image skybox_img_id = sg_alloc_image();
    state.mesh.bind.fs_images[SLOT_skybox_texture] = skybox_img_id;
    state.bind_skybox.fs_images[SLOT_skybox_texture] = skybox_img_id;
    
    lopgl_load_cubemap(&(lopgl_cubemap_request_t){
        .img_id = skybox_img_id,
        .path_right = "skybox_right.jpg",
        .path_left = "skybox_left.jpg",
        .path_top = "skybox_top.jpg",
        .path_bottom = "skybox_bottom.jpg",
        .path_front = "skybox_front.jpg",
        .path_back = "skybox_back.jpg",
        .buffer_ptr = state.cubemap_buffer,
        .buffer_offset = 1024 * 1024,
        .fail_callback = fail_callback
    });

    lopgl_load_obj(&(lopgl_obj_request_t){
        .path = "backpack.obj",
        .callback = load_obj_callback,
        .fail_callback = fail_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .model = HMM_Mat4d(1.0f),
        .view = view,
        .projection = projection
    };
    
    if (state.mesh.face_count > 0) {
        sg_apply_pipeline(state.mesh.pip);
        sg_apply_bindings(&state.mesh.bind);

        fs_params_t fs_params = {
            .camera_pos = lopgl_camera_position()
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));
        sg_draw(0, 3 * state.mesh.face_count, 1);
    }

    // remove translation from view matrix
    vs_params.view.Elements[3][0] = 0.0f;
    vs_params.view.Elements[3][1] = 0.0f;
    vs_params.view.Elements[3][2] = 0.0f;

    sg_apply_pipeline(state.pip_skybox);
    sg_apply_bindings(&state.bind_skybox);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
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
        .window_title = "Refraction Backpack (LearnOpenGL)",
    };
}
