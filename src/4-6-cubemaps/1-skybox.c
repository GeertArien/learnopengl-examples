//------------------------------------------------------------------------------
//  Cubemaps (2)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "1-skybox.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

static const char* skybox[] = {
    "skybox_right.jpg",
    "skybox_left.jpg",
    "skybox_top.jpg",
    "skybox_bottom.jpg",
    "skybox_front.jpg",
    "skybox_back.jpg"
};

/* application state */
static struct {
    sg_pipeline pip_cube;
    sg_pipeline pip_skybox;
    sg_bindings bind_cube;
    sg_bindings bind_skybox;
    sg_pass_action pass_action;
    uint8_t file_bufferr[1024 * 1024];
    uint8_t file_buffer[6 * 1024 * 1024];
    int file_sizes[6];
    int active_requests;
} state;

static void fail_callback() {
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } }
    };
}

/* The fetch-callback is called by sokol_fetch.h when the data is loaded,
   or when an error has occurred.
*/
static void skybox_fetch_callback(const sfetch_response_t* response) {
    int index = *(int*)response->user_data;

    if (response->fetched) {
        state.file_sizes[index] = response->fetched_size;
        --state.active_requests;
    }
    else if (response->failed) {
        fail_callback();
    }

    if (state.active_requests == 0) {
        int img_width, img_height, num_channels;
        const int desired_channels = 4;
        stbi_uc* pixels_ptrs[6];
        bool valid = true;
        sg_image_content img_content;

        for (int i = 0; i < 6; ++i) {
            pixels_ptrs[i] = stbi_load_from_memory(
                state.file_buffer + i * 1024 * 1024,
                state.file_sizes[i],
                &img_width, &img_height,
                &num_channels, desired_channels);    

            img_content.subimage[i][0].ptr = pixels_ptrs[i];
            img_content.subimage[i][0].size = img_width * img_height * num_channels;
        }
        
        if (valid) {
            /* initialize the sokol-gfx texture */
            sg_init_image(state.bind_skybox.fs_images[SLOT_skybox_texture], &(sg_image_desc){
                .type = SG_IMAGETYPE_CUBE,
                .width = img_width,
                .height = img_height,
                /* set pixel_format to RGBA8 for WebGL */
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
                .wrap_w = SG_WRAP_CLAMP_TO_EDGE,
                .min_filter = SG_FILTER_LINEAR,
                .mag_filter = SG_FILTER_LINEAR,
                .content = img_content
            });
        }

        for (int i = 0; i < 6; ++i) {
            stbi_image_free(pixels_ptrs[i]);
        }
    }
}

static void init(void) {
    lopgl_setup();

    float cube_vertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .content = cube_vertices,
        .label = "cube-vertices"
    });
    
    state.bind_cube.vertex_buffers[0] = cube_buffer;

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
    state.pip_cube = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(simple_shader_desc()),
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .label = "cube-pipeline"
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

    sg_image container_img_id = sg_alloc_image();
    state.bind_cube.fs_images[SLOT_diffuse_texture] = container_img_id;
    sg_image skybox_img_id = sg_alloc_image();
    state.bind_skybox.fs_images[SLOT_skybox_texture] = skybox_img_id;

    lopgl_load_image(&(lopgl_image_request_t){
            .path = "container.jpg",
            .img_id = container_img_id,
            .buffer_ptr = state.file_bufferr,
            .buffer_size = sizeof(state.file_bufferr),
            .fail_callback = fail_callback
    });

    state.active_requests = 6;

    for (int i = 0; i < 6; ++i) {
        sfetch_send(&(sfetch_request_t){
            .path = skybox[i],
            .callback = skybox_fetch_callback,
            .buffer_ptr = state.file_buffer + (i * 1024 * 1024),
            .buffer_size = 1024 * 1024,
            .user_data_ptr = &i,
            .user_data_size = sizeof(i)
        });
    }
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
    
    sg_apply_pipeline(state.pip_cube);
    sg_apply_bindings(&state.bind_cube);

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    // remove translation from view matrix
    vs_params.view.Columns[3][0] = 0.0f;
    vs_params.view.Columns[3][1] = 0.0f;
    vs_params.view.Columns[3][2] = 0.0f;

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
        .window_title = "Skybox (LearnOpenGL)",
    };
}
