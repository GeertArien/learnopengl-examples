//------------------------------------------------------------------------------
//  Normal Mapping (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "2-tangent-space.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    hmm_vec3 light_pos;
    uint8_t file_buffer[1024 * 1024];
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

hmm_vec3 computeTangent(hmm_vec3 pos0, hmm_vec3 pos1, hmm_vec3 pos2, hmm_vec2 uv0, hmm_vec2 uv1, hmm_vec2 uv2) {
    hmm_vec3 edge0 = HMM_SubtractVec3(pos1, pos0);
    hmm_vec3 edge1 = HMM_SubtractVec3(pos2, pos0);
    hmm_vec2 delta_uv0 = HMM_SubtractVec2(uv1, uv0);
    hmm_vec2 delta_uv1 = HMM_SubtractVec2(uv2, uv0);

    float f = 1.f / (delta_uv0.X * delta_uv1.Y - delta_uv1.X * delta_uv0.Y);

    float x = f * (delta_uv1.Y * edge0.X - delta_uv0.Y * edge1.X);
    float y = f * (delta_uv1.Y * edge0.Y - delta_uv0.Y * edge1.Y);
    float z = f * (delta_uv1.Y * edge0.Z - delta_uv0.Y * edge1.Z);

    return HMM_Vec3(x, y, z);
}

static void init(void) {
    lopgl_setup();

    state.light_pos = HMM_Vec3(.5f, 1.f, .3f);

    /* positions */
    hmm_vec3 pos0 = HMM_Vec3(-1.f,  1.f, 0.f);
    hmm_vec3 pos1 = HMM_Vec3(-1.f, -1.f, 0.f);
    hmm_vec3 pos2 = HMM_Vec3( 1.f, -1.f, 0.f);
    hmm_vec3 pos3 = HMM_Vec3( 1.f,  1.f, 0.f);
    /* texture coordinates */
    hmm_vec2 uv0 = HMM_Vec2(0.f, 1.f);
    hmm_vec2 uv1 = HMM_Vec2(0.f, 0.f);
    hmm_vec2 uv2 = HMM_Vec2(1.f, 0.f);  
    hmm_vec2 uv3 = HMM_Vec2(1.f, 1.f);
    /* normal vector */
    hmm_vec3 nm = HMM_Vec3(0.f, 0.f, 1.f);
    /* tangents */
    hmm_vec3 ta0 = computeTangent(pos0, pos1, pos2, uv0, uv1, uv2);
    hmm_vec3 ta1 = computeTangent(pos0, pos2, pos3, uv0, uv2, uv3);

    /* we can compute the bitangent in the vertex shader by taking 
       the cross product of the normal and tangent */
    float quad_vertices[] = {
        // positions            // normal         // texcoords  // tangent           
        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta0.X, ta0.Y, ta0.Z,
        pos1.X, pos1.Y, pos1.Z, nm.X, nm.Y, nm.Z, uv1.X, uv1.Y, ta0.X, ta0.Y, ta0.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta0.X, ta0.Y, ta0.Z,

        pos0.X, pos0.Y, pos0.Z, nm.X, nm.Y, nm.Z, uv0.X, uv0.Y, ta1.X, ta1.Y, ta1.Z,
        pos2.X, pos2.Y, pos2.Z, nm.X, nm.Y, nm.Z, uv2.X, uv2.Y, ta1.X, ta1.Y, ta1.Z,
        pos3.X, pos3.Y, pos3.Z, nm.X, nm.Y, nm.Z, uv3.X, uv3.Y, ta1.X, ta1.Y, ta1.Z
    };

    sg_buffer quad_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(quad_vertices),
        .content = quad_vertices,
        .label = "quad-vertices"
    });
    
    state.bind.vertex_buffers[0] = quad_buffer;

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
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2,
                [ATTR_vs_a_tangent].format = SG_VERTEXFORMAT_FLOAT3,
            }
        },
        .label = "object-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };

    sg_image img_id_diffuse = sg_alloc_image();
    state.bind.fs_images[SLOT_diffuse_map] = img_id_diffuse;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "brickwall.jpg",
            .img_id = img_id_diffuse,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });

    sg_image img_id_normal = sg_alloc_image();
    state.bind.fs_images[SLOT_normal_map] = img_id_normal;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "brickwall_normal.jpg",
            .img_id = img_id_normal,
            .buffer_ptr = state.file_buffer,
            .buffer_size = sizeof(state.file_buffer),
            .fail_callback = fail_callback
    });
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);
    /* rotate the quad to show normal mapping from multiple directions */
    hmm_mat4 model = HMM_Rotate((float)stm_sec(stm_now()) * -10.f, HMM_NormalizeVec3(HMM_Vec3(1.f, 0.f, 1.f)));

    vs_params_t vs_params = {
        .view = view,
        .projection = projection,
        .model = model,
        .view_pos = lopgl_camera_position(),
        .light_pos = state.light_pos
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

    sg_draw(0, 6, 1);

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
        .window_title = "Tangent Space (LearnOpenGL)",
    };
}
