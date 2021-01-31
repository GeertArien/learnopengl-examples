//------------------------------------------------------------------------------
//  Normal Mapping (3)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "3-complex-object.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

static const char* filename = "backpack.obj";

typedef struct vertex {
    hmm_vec3 pos;
    hmm_vec3 normal;
    hmm_vec2 tex_coords;
    hmm_vec3 tangent;
} vertex_t;

typedef struct mesh_t {
    sg_pipeline pip;
    sg_bindings bind;
    unsigned int face_count;
} mesh_t;

/* application state */
static struct {
    mesh_t mesh;
    sg_pass_action pass_action;
    hmm_vec3 light_pos;
    uint8_t file_buffer[32 * 1024 * 1024];
    float vertex_buffer[70000 * 3 * 11]
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

hmm_vec3 computeTangent(vertex_t* v0, vertex_t* v1, vertex_t* v2) {
    hmm_vec3 edge0 = HMM_SubtractVec3(v1->pos, v0->pos);
    hmm_vec3 edge1 = HMM_SubtractVec3(v2->pos, v0->pos);
    hmm_vec2 delta_uv0 = HMM_SubtractVec2(v1->tex_coords, v0->tex_coords);
    hmm_vec2 delta_uv1 = HMM_SubtractVec2(v2->tex_coords, v0->tex_coords);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);

    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

static vertex_t getVertex(unsigned int index, fastObjMesh* mesh) {
    fastObjIndex fo_vertex = mesh->indices[index];

    unsigned int v_pos = fo_vertex.p * 3;
    unsigned int n_pos = fo_vertex.n * 3;
    unsigned int t_pos = fo_vertex.t * 2;

    vertex_t vertex;
    memcpy(&vertex.pos, mesh->positions + v_pos, 3 * sizeof(float));
    memcpy(&vertex.normal, mesh->normals + n_pos, 3 * sizeof(float));
    memcpy(&vertex.tex_coords, mesh->texcoords + t_pos, 2 * sizeof(float));
    return vertex;
}

static void load_obj_callback(lopgl_obj_response_t* response) {
    fastObjMesh* mesh = response->mesh;
    state.mesh.face_count = mesh->face_count;

    for (unsigned int i = 0; i < mesh->face_count; ++i) {
        unsigned int index = i * 3;
        vertex_t v0 = getVertex(index, mesh);
        vertex_t v1 = getVertex(index + 1, mesh);
        vertex_t v2 = getVertex(index + 2, mesh);

        v0.tangent = v1.tangent = v2.tangent = computeTangent(&v0, &v1, &v2);

        size_t si = sizeof(vertex_t);

        unsigned int pos = i * 3 * 11;
        memcpy(state.vertex_buffer + pos, &v0, sizeof(vertex_t));
        memcpy(state.vertex_buffer + pos + 1 * 11, &v1, sizeof(vertex_t));
        memcpy(state.vertex_buffer + pos + 2 * 11, &v2, sizeof(vertex_t));
    }

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = mesh->face_count * 3 * 11 * sizeof(float),
        .content = state.vertex_buffer,
        .label = "backpack-vertices"
    });
    
    state.mesh.bind.vertex_buffers[0] = cube_buffer;
    
    sg_image img_id_diffuse = sg_alloc_image();
    sg_image img_id_specular = sg_alloc_image();
    sg_image img_id_normal = sg_alloc_image();
    state.mesh.bind.fs_images[SLOT_diffuse_map] = img_id_diffuse;
    state.mesh.bind.fs_images[SLOT_specular_map] = img_id_specular;
    state.mesh.bind.fs_images[SLOT_normal_map] = img_id_normal;

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

    lopgl_load_image(&(lopgl_image_request_t){
        .path = mesh->materials[0].map_bump.name,
        .img_id = img_id_normal,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .fail_callback = fail_callback
    });
}

static void init(void) {
    lopgl_setup();

    state.light_pos = HMM_Vec3(.5f, 1.f, .3f);

    /* create shader from code-generated sg_shader_desc */
    sg_shader shd = sg_make_shader(blinn_phong_shader_desc());

    /* create a pipeline object for object */
    state.mesh.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_normal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_a_tangent].format = SG_VERTEXFORMAT_FLOAT3,
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
            .view = view,
            .projection = projection,
            .model = HMM_Mat4d(1.f),
            .view_pos = lopgl_camera_position(),
            .light_pos = state.light_pos
        };

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
        .gl_force_gles2 = true,
        .window_title = "Complex Object (LearnOpenGL)",
    };
}
