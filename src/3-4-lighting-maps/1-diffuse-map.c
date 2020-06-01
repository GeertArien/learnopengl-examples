//------------------------------------------------------------------------------
//  Lighting Maps (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#include "sokol_fetch.h"
#include "hmm/HandmadeMath.h"
#include "1-diffuse-map.glsl.h"
#include "ui/ui.h"
#include "../utility/camera.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

/* application state */
static struct {
    sg_pipeline pip_object;
    sg_pipeline pip_light;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint64_t last_time;
    uint64_t delta_time;
    bool first_mouse;
    float last_x;
    float last_y;
    struct cam_desc camera;
    hmm_vec3 light_pos;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .gl_force_gles2 = sapp_gles2(),
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
    ui_setup(1);

    /* initialize sokol_time */
    stm_setup();

    // hide mouse cursor
    sapp_show_mouse(false);

    // set default camera configuration
    state.first_mouse = true;
    state.camera = create_camera(HMM_Vec3(0.0f, 0.0f,  3.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), -90.f, 0.0f);

    // set object and light configuration
    state.light_pos = HMM_Vec3(1.2f, 1.0f, 2.0f);

    float vertices[] = {
        // positions        // normals

        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };
    
    state.bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "cube-vertices"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader phong_shd = sg_make_shader(phong_shader_desc());

    /* create a pipeline object for object */
    state.pip_object = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = phong_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "object-pipeline"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader light_cube_shd = sg_make_shader(light_cube_shader_desc());

    /* create a pipeline object for light cube */
    state.pip_light = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = light_cube_shd,
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3
            },
            .buffers[0].stride = 24
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
            .depth_write_enabled = true,
        },
        .label = "light-cube-pipeline"
    });
    
    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    state.delta_time = stm_laptime(&state.last_time);

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    hmm_mat4 view = get_view_matrix(&state.camera);
    hmm_mat4 projection = HMM_Perspective(state.camera.zoom, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    sg_apply_pipeline(state.pip_object);
    sg_apply_bindings(&state.bind);

    vs_params.model = HMM_Mat4d(1.f);;
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

    fs_params_t fs_params = {
        .viewPos = state.camera.position,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));

    fs_material_t fs_material = {
        .ambient = HMM_Vec3(1.0f, 0.5f, 0.31f),
        .diffuse = HMM_Vec3(1.0f, 0.5f, 0.31f),
        .specular = HMM_Vec3(0.5f, 0.5f, 0.5f),
        .shininess = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_material, &fs_material, sizeof(fs_material));

    float time = (float) stm_sec(stm_now());

    hmm_vec3 light_color;
    light_color.X = sinf(time * 2.0f);
    light_color.Y = sinf(time * 0.7f);
    light_color.W = sinf(time * 1.3f);
    
    fs_light_t fs_light = {
        .position = state.light_pos,
        .ambient = HMM_MultiplyVec3(light_color, HMM_Vec3(0.2f, 0.2f, 0.2f)),
        .diffuse = HMM_MultiplyVec3(light_color, HMM_Vec3(0.5f, 0.5f, 0.5f)),
        .specular = HMM_Vec3(1.0f, 1.0f, 1.0f)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_light, &fs_light, sizeof(fs_light));

    sg_draw(0, 36, 1);

    sg_apply_pipeline(state.pip_light);
    sg_apply_bindings(&state.bind);
    vs_params.model = HMM_Translate(state.light_pos);
    vs_params.model = HMM_MultiplyMat4(vs_params.model, HMM_Scale(HMM_Vec3(0.2f, 0.2f, 0.2f)));
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    ui_draw(NULL);
    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }

        if (e->key_code == SAPP_KEYCODE_SPACE) {
            bool mouse_shown = sapp_mouse_shown();
            sapp_show_mouse(!mouse_shown);
            toggle_camera_movement(&state.camera);
        }

        float f_delta_time = (float) stm_sec(state.delta_time);

        if (e->key_code == SAPP_KEYCODE_W) {
            process_keyboard(&state.camera, CAM_MOV_FORWARD, f_delta_time);
        }
        if (e->key_code == SAPP_KEYCODE_S) {
            process_keyboard(&state.camera, CAM_MOV_BACKWARD, f_delta_time);
        }
        if (e->key_code == SAPP_KEYCODE_A) {
            process_keyboard(&state.camera, CAM_MOV_LEFT, f_delta_time);
        }
        if (e->key_code == SAPP_KEYCODE_D) {
            process_keyboard(&state.camera, CAM_MOV_RIGHT, f_delta_time);
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if(state.first_mouse) {
            state.last_x = e->mouse_x;
            state.last_y = e->mouse_y;
            state.first_mouse = false;
        }
    
        float xoffset = e->mouse_x - state.last_x;
        float yoffset = state.last_y - e->mouse_y; 
        state.last_x = e->mouse_x;
        state.last_y = e->mouse_y;

        process_mouse_movement(&state.camera, xoffset, yoffset);
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        process_mouse_scroll(&state.camera, e->scroll_y);
    }

    ui_event(e);
}

void cleanup(void) {
    ui_shutdown();
    sg_shutdown();
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
        .window_title = "Diffuse Map (LearnOpenGL)",
    };
}
