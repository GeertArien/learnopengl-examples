//------------------------------------------------------------------------------
//  Light Casters (3)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "sokol_fetch.h"
#include "stb/stb_image.h"
#include "hmm/HandmadeMath.h"
#include "3-spot-light.glsl.h"
#include "ui/ui.h"
#include "../utility/camera.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    uint64_t last_time;
    uint64_t delta_time;
    bool first_mouse;
    float last_x;
    float last_y;
    struct cam_desc camera;
    hmm_vec3 cube_positions[10];
    uint8_t file_buffer[512 * 1024];
} state;

static void fetch_callback(const sfetch_response_t*);

static void init(void) {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });
    ui_setup(1);

    /* setup sokol-fetch
        The 1 channel and 1 lane configuration essentially serializes
        IO requests. Which is just fine for this example. */
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 2,
        .num_channels = 1,
        .num_lanes = 1
    });

    /* Allocate an image handle, but don't actually initialize the image yet,
       this happens later when the asynchronous file load has finished.
       Any draw calls containing such an "incomplete" image handle
       will be silently dropped.
    */
    state.bind.fs_images[SLOT_diffuse_texture] = sg_alloc_image();
    state.bind.fs_images[SLOT_specular_texture] = sg_alloc_image();

    /* flip images vertically after loading */
    stbi_set_flip_vertically_on_load(true);  

    /* initialize sokol_time */
    stm_setup();

    // hide mouse cursor
    sapp_show_mouse(false);

    // set default camera configuration
    state.first_mouse = true;
    state.camera = create_camera(HMM_Vec3(0.0f, 0.0f,  3.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), -90.f, 0.0f);

    state.cube_positions[0] = HMM_Vec3( 0.0f,  0.0f,  0.0f);
    state.cube_positions[1] = HMM_Vec3( 2.0f,  5.0f, -15.0f);
    state.cube_positions[2] = HMM_Vec3(-1.5f, -2.2f, -2.5f);
    state.cube_positions[3] = HMM_Vec3(-3.8f, -2.0f, -12.3f);
    state.cube_positions[4] = HMM_Vec3( 2.4f, -0.4f, -3.5f);
    state.cube_positions[5] = HMM_Vec3(-1.7f,  3.0f, -7.5f);
    state.cube_positions[6] = HMM_Vec3( 1.3f, -2.0f, -2.5f);
    state.cube_positions[7] = HMM_Vec3( 1.5f,  2.0f, -2.5f);
    state.cube_positions[8] = HMM_Vec3( 1.5f,  0.2f, -1.5f);
    state.cube_positions[9] = HMM_Vec3(-1.3f,  1.0f, -1.5f);

    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
        .label = "cube-vertices"
    });
    
    state.bind.vertex_buffers[0] = cube_buffer;

    /* create shader from code-generated sg_shader_desc */
    sg_shader phong_shd = sg_make_shader(phong_shader_desc());

    /* create a pipeline object for object */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = phong_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_aTexCoords].format = SG_VERTEXFORMAT_FLOAT2
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

    sg_image img_id_diffuse = state.bind.fs_images[SLOT_diffuse_texture];
    sg_image img_id_specular = state.bind.fs_images[SLOT_specular_texture];

    /* start loading the PNG file */
    sfetch_send(&(sfetch_request_t){
        .path = "container2.png",
        .callback = fetch_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .user_data_ptr = &img_id_diffuse,
        .user_data_size = sizeof(img_id_diffuse)
    });

    sfetch_send(&(sfetch_request_t){
        .path = "container2_specular.png",
        .callback = fetch_callback,
        .buffer_ptr = state.file_buffer,
        .buffer_size = sizeof(state.file_buffer),
        .user_data_ptr = &img_id_specular,
        .user_data_size = sizeof(img_id_specular)
    });
}

/* The fetch-callback is called by sokol_fetch.h when the data is loaded,
   or when an error has occurred.
*/
static void fetch_callback(const sfetch_response_t* response) {
    if (response->fetched) {
        /* the file data has been fetched, since we provided a big-enough
           buffer we can be sure that all data has been loaded here
        */
        int img_width, img_height, num_channels;
        const int desired_channels = 4;
        stbi_uc* pixels = stbi_load_from_memory(
            response->buffer_ptr,
            (int)response->fetched_size,
            &img_width, &img_height,
            &num_channels, desired_channels);
        if (pixels) {
            /* we attached the image slot value to the request's user_data */
            sg_image img_id = *(sg_image*)response->user_data;
            /* initialize the sokol-gfx texture */
            sg_init_image(img_id, &(sg_image_desc){
                .width = img_width,
                .height = img_height,
                /* set pixel_format to RGBA8 for WebGL */
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_REPEAT,
                .wrap_v = SG_WRAP_REPEAT,
                .min_filter = SG_FILTER_LINEAR,
                .mag_filter = SG_FILTER_LINEAR,
                .content.subimage[0][0] = {
                    .ptr = pixels,
                    .size = img_width * img_height * 4,
                }
            });
            stbi_image_free(pixels);
        }
    }
    else if (response->failed) {
        // if loading the file failed, set clear color to red
        state.pass_action = (sg_pass_action) {
            .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
        };
    }
}

void frame(void) {
    sfetch_dowork();
    state.delta_time = stm_laptime(&state.last_time);

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    fs_params_t fs_params = {
        .viewPos = state.camera.position,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params, sizeof(fs_params));

    fs_material_t fs_material = {
        .shininess = 32.0f,
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_material, &fs_material, sizeof(fs_material));
    
    fs_light_t fs_light = {
        .position = state.camera.position,
        .direction = state.camera.front,
        .cut_off = HMM_COSF(HMM_ToRadians(12.5f)),
        .ambient = HMM_Vec3(0.2f, 0.2f, 0.2f),
        .diffuse = HMM_Vec3(0.5f, 0.5f, 0.5f),
        .specular = HMM_Vec3(1.0f, 1.0f, 1.0f),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_light, &fs_light, sizeof(fs_light));

    hmm_mat4 view = get_view_matrix(&state.camera);
    hmm_mat4 projection = HMM_Perspective(state.camera.zoom, (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .view = view,
        .projection = projection
    };

    for(size_t i = 0; i < 10; i++) {
        hmm_mat4 model = HMM_Translate(state.cube_positions[i]);
        float angle = 20.0f * i; 
        model = HMM_MultiplyMat4(model, HMM_Rotate(angle, HMM_Vec3(1.0f, 0.3f, 0.5f)));
        vs_params.model = model;
        sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));

        sg_draw(0, 36, 1);
    }

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
        .window_title = "Spot Light (LearnOpenGL)",
    };
}
