//------------------------------------------------------------------------------
//  Instancing (4)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "4-asteroid-field-instanced.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"
#include "fast_obj/lopgl_fast_obj.h"

#define ASTEROID_COUNT 100000

typedef struct mesh_t {
    sg_pipeline pip;
    sg_bindings bind;
    unsigned int face_count;
} mesh_t;

/* application state */
static struct {
    mesh_t planet;
    mesh_t rock;
    hmm_mat4 rock_transforms[ASTEROID_COUNT];
    sg_pass_action pass_action;
    uint8_t file_buffer_planet[1024 * 1024];
    uint8_t file_buffer_rock[1024 * 1024];
    uint8_t file_buffer_img[3 * 1024 * 1024];
    float vertex_buffer[1024 * 8 * 3];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void load_obj_callback(lopgl_obj_response_t* response) {
    mesh_t* mesh = (mesh_t*) response->user_data_ptr;
    mesh->face_count = response->mesh->face_count;

    for (unsigned int i = 0; i < mesh->face_count * 3; ++i) {
        fastObjIndex vertex = response->mesh->indices[i];

        unsigned int pos = i * 8;
        unsigned int v_pos = vertex.p * 3;
        unsigned int n_pos = vertex.n * 3;
        unsigned int t_pos = vertex.t * 2;

        memcpy(state.vertex_buffer + pos, response->mesh->positions + v_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 3, response->mesh->normals + n_pos, 3 * sizeof(float));
        memcpy(state.vertex_buffer + pos + 6, response->mesh->texcoords + t_pos, 2 * sizeof(float));
    }

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = mesh->face_count * 3 * 8 * sizeof(float),
        .content = state.vertex_buffer,
        .label = "mesh-vertices"
    });
    
    mesh->bind.vertex_buffers[0] = cube_buffer;
    sg_image img_id = sg_alloc_image();
    mesh->bind.fs_images[SLOT_diffuse_texture] = img_id;

    lopgl_load_image(&(lopgl_image_request_t){
        .path = response->mesh->materials[0].map_Kd.name,
        .img_id = img_id,
        /* Webgl 1.0 does not support repeat for textures that are not a power of two in size */
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .buffer_ptr = state.file_buffer_img,
        .buffer_size = sizeof(state.file_buffer_img),
        .fail_callback = fail_callback
    });
}

static void init(void) {
    lopgl_setup();

    lopgl_orbital_cam_desc_t orbital_desc = lopgl_get_orbital_cam_desc();
    orbital_desc.distance = 155.f;
    lopgl_set_orbital_cam(&orbital_desc);

    lopgl_fp_cam_desc_t fp_desc = lopgl_get_fp_cam_desc();
    fp_desc.position.Z = 150.f;
    lopgl_set_fp_cam(&fp_desc);

    /* create shader from code-generated sg_shader_desc */
    sg_shader planet_shd = sg_make_shader(planet_shader_desc());

    /* create a pipeline object for the planet  */
    state.planet.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = planet_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_planet_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_planet_a_tex_coords] = {.format = SG_VERTEXFORMAT_FLOAT2, .offset = 24 }
            },
            .buffers[0].stride = 32
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "planet-pipeline"
    });

    sg_shader rock_shd = sg_make_shader(rock_shader_desc());

    /* create a pipeline object for the asteroids  */
    state.rock.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = rock_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_rock_a_pos] = {.format = SG_VERTEXFORMAT_FLOAT3, .buffer_index = 0 },
                [ATTR_vs_rock_a_tex_coords] = {.format = SG_VERTEXFORMAT_FLOAT2, .offset = 24, .buffer_index = 0 },
                [ATTR_vs_rock_instance_mat0] = {.format = SG_VERTEXFORMAT_FLOAT4, .offset = 0, .buffer_index = 1},
                [ATTR_vs_rock_instance_mat1] = {.format = SG_VERTEXFORMAT_FLOAT4, .offset = 16, .buffer_index = 1},
                [ATTR_vs_rock_instance_mat2] = {.format = SG_VERTEXFORMAT_FLOAT4, .offset = 32, .buffer_index = 1},
                [ATTR_vs_rock_instance_mat3] = {.format = SG_VERTEXFORMAT_FLOAT4, .offset = 48, .buffer_index = 1},
            },
            .buffers[0] = {.stride = 32, .step_func = SG_VERTEXSTEP_PER_VERTEX },
            /* vertex buffer at slot 1 must step per instance */
            .buffers[1] = {.stride = 64, .step_func = SG_VERTEXSTEP_PER_INSTANCE }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "rock-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    lopgl_load_obj(&(lopgl_obj_request_t){
        .path = "planet.obj",
        .callback = load_obj_callback,
        .fail_callback = fail_callback,
        .buffer_ptr = state.file_buffer_planet,
        .buffer_size = sizeof(state.file_buffer_planet),
        .user_data_ptr = &state.planet
    });

    lopgl_load_obj(&(lopgl_obj_request_t){
        .path = "rock.obj",
        .callback = load_obj_callback,
        .fail_callback = fail_callback,
        .buffer_ptr = state.file_buffer_rock,
        .buffer_size = sizeof(state.file_buffer_rock),
        .user_data_ptr = &state.rock
    });

    srand(stm_now()); // initialize random seed	
    float radius = 100.f;
    float offset = 25.f;
    for(unsigned int i = 0; i < ASTEROID_COUNT; i++) {
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)ASTEROID_COUNT * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = HMM_SinF(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = HMM_CosF(angle) * radius + displacement;
        hmm_mat4 model = HMM_Translate(HMM_Vec3(x, y, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        model = HMM_MultiplyMat4(model, HMM_Scale(HMM_Vec3(scale, scale, scale)));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rot_angle = (rand() % 360);
        model = HMM_MultiplyMat4(model, HMM_Rotate(rot_angle, HMM_Vec3(0.4f, 0.6f, 0.8f)));

        // 4. now add to list of matrices
        state.rock_transforms[i] = model;
    }

    sg_buffer transform_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = ASTEROID_COUNT * sizeof(hmm_mat4),
        .content = state.rock_transforms,
        .label = "rock-transforms"
    });
    
    state.rock.bind.vertex_buffers[1] = transform_buffer;
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 1000.0f);

    if (state.planet.face_count > 0) {
        sg_apply_pipeline(state.planet.pip);
        sg_apply_bindings(&state.planet.bind);

        hmm_mat4 model = HMM_Translate(HMM_Vec3(0.f, -3.f, 0.f));
        model = HMM_MultiplyMat4(model, HMM_Scale(HMM_Vec3(4.f, 4.f, 4.f)));

        vs_params_planet_t vs_params = {
            .model = model,
            .view = view,
            .projection = projection
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_planet, &vs_params, sizeof(vs_params));

        sg_draw(0, state.planet.face_count * 3, 1);
    }

    if (state.rock.face_count > 0) {
        sg_apply_pipeline(state.rock.pip);
        sg_apply_bindings(&state.rock.bind);

        vs_params_rock_t vs_params = {
            .view = view,
            .projection = projection
        };

        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params_rock, &vs_params, sizeof(vs_params));
        sg_draw(0, state.rock.face_count * 3, ASTEROID_COUNT);
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
        .window_title = "Asteroid Field Instanced (LearnOpenGL)",
    };
}
