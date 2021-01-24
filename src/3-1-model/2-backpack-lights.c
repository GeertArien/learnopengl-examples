//------------------------------------------------------------------------------
//  Model Loading (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "2-backpack-lights.glsl.h"
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
    hmm_vec4 light_positions[4];
    uint8_t file_buffer[16 * 1024 * 1024];
    float vertex_buffer[70000 * 3 * 8];
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

        unsigned int pos = i * 8;
        unsigned int v_pos = vertex.p * 3;
        unsigned int n_pos = vertex.n * 3;
        unsigned int t_pos = vertex.t * 2;

        memcpy(state.vertex_buffer + pos, mesh->positions + v_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 3, mesh->normals + n_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 6, mesh->texcoords + t_pos, 2 * sizeof(float));
    }

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = mesh->face_count * 3 * 8 * sizeof(float),
        .content = state.vertex_buffer,
        .label = "backpack-vertices"
    });
    
    state.mesh.bind.vertex_buffers[0] = cube_buffer;
    
    sg_image img_id_diffuse = sg_alloc_image();
    sg_image img_id_specular = sg_alloc_image();
    state.mesh.bind.fs_images[SLOT_diffuse_texture] = img_id_diffuse;
    state.mesh.bind.fs_images[SLOT_specular_texture] = img_id_specular;

    lopgl_load_image(&(lopgl_image_request_t){
        .path = mesh->materials[0].map_Kd.name,
        .img_id = img_id_diffuse,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });

    lopgl_load_image(&(lopgl_image_request_t){
        .path = mesh->materials[0].map_Ks.name,
        .img_id = img_id_specular,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });
}

static void init(void) {
    lopgl_setup();

    // positions of the point lights
    state.light_positions[0] = HMM_Vec4( 0.7f,  0.2f,  2.0f, 1.0f);
    state.light_positions[1] = HMM_Vec4( 2.3f, -3.3f, -4.0f, 1.0f);
    state.light_positions[2] = HMM_Vec4(-4.0f,  2.0f, -12.0f, 1.0f);
    state.light_positions[3] = HMM_Vec4( 0.0f,  0.0f, -3.0f, 1.0f);

    /* create shader from code-generated sg_shader_desc */
    sg_shader phong_shd = sg_make_shader(phong_shader_desc());

    /* create a pipeline object for object */
    state.mesh.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = phong_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
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
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    if (state.mesh.face_count > 0) {
        sg_apply_pipeline(state.mesh.pip);
        sg_apply_bindings(&state.mesh.bind);

        hmm_mat4 view = lopgl_view_matrix();
        hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

        vs_params_t vs_params = {
            .model = HMM_Mat4d(1.f),
            .view = view,
            .projection = projection
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

        fs_params_t fs_params = {
            .view_pos = lopgl_camera_position(),
            .material_shininess = 32.0f,
        };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));
        
        fs_dir_light_t fs_dir_light = {
            .direction = HMM_Vec3(-0.2f, -1.0f, -0.3f),
            .ambient = HMM_Vec3(0.05f, 0.05f, 0.05f),
            .diffuse = HMM_Vec3(0.4f, 0.4f, 0.4f),
            .specular = HMM_Vec3(0.5f, 0.5f, 0.5f)
        };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_dir_light, &fs_dir_light, sizeof(fs_dir_light));

        fs_point_lights_t fs_point_lights = {
            .position[0]    = state.light_positions[0],
            .ambient[0]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
            .diffuse[0]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
            .specular[0]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
            .attenuation[0] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
            .position[1]    = state.light_positions[1],
            .ambient[1]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
            .diffuse[1]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
            .specular[1]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
            .attenuation[1] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
            .position[2]    = state.light_positions[2],
            .ambient[2]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
            .diffuse[2]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
            .specular[2]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
            .attenuation[2] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f),
            .position[3]    = state.light_positions[3],
            .ambient[3]     = HMM_Vec4(0.05f, 0.05f, 0.05f, 0.0f),
            .diffuse[3]     = HMM_Vec4(0.8f, 0.8f, 0.8f, 0.0f),
            .specular[3]    = HMM_Vec4(1.0f, 1.0f, 1.0f, 0.0f),
            .attenuation[3] = HMM_Vec4(1.0f, 0.09f, 0.032f, 0.0f)
        };
        sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_point_lights, &fs_point_lights, sizeof(fs_point_lights_t));
        
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
        .gl_force_gles2 = true,
        .window_title = "Backpack Multiple Lights (LearnOpenGL)",
    };
}
