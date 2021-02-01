#ifndef LOPGL_APP_INCLUDED
#define LOPGL_APP_INCLUDED

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "../libs/hmm/HandmadeMath.h"
#include "../libs/fast_obj/lopgl_fast_obj.h"

/*
    TODO:
        - add asserts to check setup has been called
        - add default values for all structs
        - use canary?
        - improve structure and add/update documentation
        - add support for obj's with multiple materials
        - add define to select functionality and reduce binary size
        - use get/set to configure cameras
*/

typedef struct lopgl_orbital_cam_desc {
    hmm_vec3 target; 
    hmm_vec3 up;
    float pitch;
    float heading;
    float distance;
    // camera limits
    float min_pitch;
	float max_pitch;
	float min_dist;
	float max_dist;
    // control options
    float rotate_speed;
	float zoom_speed;
} lopgl_orbital_cam_desc_t;

typedef struct lopgl_fp_cam_desc {
    hmm_vec3 position;
    hmm_vec3 world_up;
    float yaw;
    float pitch;
    float zoom;
    // limits
    float min_pitch;
	float max_pitch;
	float min_zoom;
	float max_zoom;
    // control options
    float movement_speed;
    float aim_speed;
    float zoom_speed;
} lopgl_fp_cam_desc_t;

/* response fail callback function signature */
typedef void(*lopgl_fail_callback_t)();

typedef struct lopgl_obj_response_t {
    uint32_t _start_canary;
    fastObjMesh* mesh; 
    void* user_data_ptr;
    uint32_t _end_canary;
} lopgl_obj_response_t;

typedef void(*lopgl_obj_request_callback_t)(lopgl_obj_response_t*);

/* request parameters passed to lopgl_load_image() */
typedef struct lopgl_image_request_t {
    uint32_t _start_canary;
    const char* path;                       /* filesystem path or HTTP URL (required) */
    sg_image img_id;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    void* buffer_ptr;                       /* buffer pointer where data will be loaded into */
    uint32_t buffer_size;                   /* buffer size in number of bytes */
    lopgl_fail_callback_t fail_callback;    /* response callback function pointer (required) */
    uint32_t _end_canary;
} lopgl_image_request_t;

/* request parameters passed to sfetch_send() */
typedef struct lopgl_obj_request_t {
    uint32_t _start_canary;
    const char* path;                       /* filesystem path or HTTP URL (required) */
    void* buffer_ptr;                       /* buffer pointer where data will be loaded into */
    uint32_t buffer_size;                   /* buffer size in number of bytes */
    const void* user_data_ptr;              /* pointer to a POD user-data block which will be memcpy'd(!) (optional) */
    lopgl_obj_request_callback_t callback;  
    lopgl_fail_callback_t fail_callback;    /* response callback function pointer (required) */
    uint32_t _end_canary;
} lopgl_obj_request_t;

typedef struct lopgl_cubemap_request_t {
    uint32_t _start_canary;
    const char* path_right;                 /* filesystem path or HTTP URL (required) */
    const char* path_left;                  /* filesystem path or HTTP URL (required) */
    const char* path_top;                   /* filesystem path or HTTP URL (required) */
    const char* path_bottom;                /* filesystem path or HTTP URL (required) */
    const char* path_front;                 /* filesystem path or HTTP URL (required) */
    const char* path_back;                  /* filesystem path or HTTP URL (required) */
    sg_image img_id;
    uint8_t* buffer_ptr;                       /* buffer pointer where data will be loaded into */
    uint32_t buffer_offset;                 /* buffer offset in number of bytes */
    lopgl_fail_callback_t fail_callback;    /* response callback function pointer (required) */
    uint32_t _end_canary;
} lopgl_cubemap_request_t;

void lopgl_setup();

void lopgl_update();

void lopgl_shutdown();

lopgl_orbital_cam_desc_t lopgl_get_orbital_cam_desc();

lopgl_fp_cam_desc_t lopgl_get_fp_cam_desc();

void lopgl_set_orbital_cam(lopgl_orbital_cam_desc_t* desc);

void lopgl_set_fp_cam(lopgl_fp_cam_desc_t* desc);

hmm_mat4 lopgl_view_matrix();

float lopgl_fov();

hmm_vec3 lopgl_camera_position();

hmm_vec3 lopgl_camera_direction();

void lopgl_handle_input(const sapp_event* e);

void lopgl_render_help();

void lopgl_load_image(const lopgl_image_request_t* request);

void lopgl_load_obj(const lopgl_obj_request_t* request);

#endif /*LOPGL_APP_INCLUDED*/


/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef LOPGL_APP_IMPL

#include "sokol_fetch.h"
#include "sokol_glue.h"
#include "sokol_time.h"

#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"
#undef SOKOL_DEBUGTEXT_IMPL

#define STB_IMAGE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "../libs/stb/stb_image.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#undef STB_IMAGE_IMPLEMENTATION


#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "../libs/hmm/HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#define FAST_OBJ_IMPLEMENTATION
#include "../libs/fast_obj/lopgl_fast_obj.h"
#undef FAST_OBJ_IMPLEMENTATION

#include <stdbool.h>

/*=== ORBITAL CAM ==================================================*/

struct orbital_cam {
    // camera config
    hmm_vec3 target;
    hmm_vec3 up;
    hmm_vec2 polar;
    float distance;
    // camera limits
    float min_pitch;
	float max_pitch;
	float min_dist;
	float max_dist;
    // control options
    float rotate_speed;
	float zoom_speed;
    bool enable_rotate;
    // internal state
    hmm_vec3 position;
};

static void update_orbital_cam_vectors(struct orbital_cam* camera);
hmm_mat4 view_matrix_orbital(struct orbital_cam* camera);
void handle_input_orbital(struct orbital_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset);
const char* help_orbital();

/*=== FP CAM =======================================================*/

struct fp_cam {
    // camera attributes
    hmm_vec3 position;
    hmm_vec3 world_up;
    float yaw;
    float pitch;
    float zoom;
    // limits
    float min_pitch;
	float max_pitch;
	float min_zoom;
	float max_zoom;
    // control options
    float movement_speed;
    float aim_speed;
    float zoom_speed;
    // control state
    bool enable_aim;
    bool move_forward;
    bool move_backward;
    bool move_left;
    bool move_right;
    // internal state
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 right;
};

static void update_fp_cam_vectors(struct fp_cam* camera);
// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
hmm_mat4 view_matrix_fp(struct fp_cam* camera);
void handle_input_fp(struct fp_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset);
void update_fp_camera(struct fp_cam* camera, float delta_time);
const char* help_fp();

/*=== APP ==========================================================*/

typedef struct _cubemap_request_t {
    sg_image img_id;
    uint8_t* buffer;
    int buffer_offset;
    int fetched_sizes[6];
    int finished_requests;
    bool failed;
    lopgl_fail_callback_t fail_callback;
} _cubemap_request_t;

typedef struct {
    struct orbital_cam orbital_cam;
    struct fp_cam fp_cam;
    bool fp_enabled;
    bool show_help;
    bool hide_ui;
    bool first_mouse;
    hmm_vec2 last_mouse;
    hmm_vec2 last_touch[SAPP_MAX_TOUCHPOINTS];
    uint64_t time_stamp;
    uint64_t frame_time;
    _cubemap_request_t cubemap_req;
} lopgl_state_t;

static lopgl_state_t _lopgl;

void lopgl_setup() {
    sg_setup(&(sg_desc){
        .context = sapp_sgcontext()
    });

    /* initialize sokol_time */
    stm_setup();

    sdtx_setup(&(sdtx_desc_t){
        .fonts = {
            [0] = sdtx_font_cpc()
        }
    });

    /* setup sokol-fetch
        The 1 channel and 1 lane configuration essentially serializes
        IO requests. Which is just fine for this example. */
    sfetch_setup(&(sfetch_desc_t){
        .max_requests = 8,
        .num_channels = 1,
        .num_lanes = 1
    });

    /* flip images vertically after loading */
    // stbi_set_flip_vertically_on_load(true);  

    lopgl_set_orbital_cam(&(lopgl_orbital_cam_desc_t) {
        .target = HMM_Vec3(0.0f, 0.0f,  0.0f),
        .up = HMM_Vec3(0.0f, 1.0f,  0.0f),
        .pitch = 0.f,
        .heading = 0.f,
        .distance = 6.f,
        .zoom_speed = .5f,
        .rotate_speed = 1.f,
        .min_dist = 1.f,
        .max_dist = 10.f,
        .min_pitch = -89.f,
        .max_pitch = 89.f
    });

    lopgl_set_fp_cam(&(lopgl_fp_cam_desc_t) {
        .position = HMM_Vec3(0.0f, 0.0f,  6.0f),
        .world_up = HMM_Vec3(0.0f, 1.0f,  0.0f),
        .yaw = -90.f,
        .pitch = 0.f,
        .zoom = 45.f,
        .movement_speed = 0.005f,
        .aim_speed = 1.f,
        .zoom_speed = .1f,
        .min_pitch = -89.f,
        .max_pitch = 89.f,
        .min_zoom = 1.f,
        .max_zoom = 45.f
    });

    _lopgl.fp_enabled = false;
    _lopgl.first_mouse = true;
    _lopgl.show_help = false;
    _lopgl.hide_ui = false;
}

void lopgl_update() {
    sfetch_dowork();

    _lopgl.frame_time = stm_laptime(&_lopgl.time_stamp);
    
    if (_lopgl.fp_enabled) {
        update_fp_camera(&_lopgl.fp_cam, stm_ms(_lopgl.frame_time));
    }
}

void lopgl_shutdown() {
    sg_shutdown();
}

lopgl_orbital_cam_desc_t lopgl_get_orbital_cam_desc() {
    return (lopgl_orbital_cam_desc_t) {
        .target = _lopgl.orbital_cam.target,
        .up = _lopgl.orbital_cam.up,
        .pitch = _lopgl.orbital_cam.polar.X,
        .heading = _lopgl.orbital_cam.polar.Y,
        .distance = _lopgl.orbital_cam.distance,
        .zoom_speed = _lopgl.orbital_cam.zoom_speed,
        .rotate_speed = _lopgl.orbital_cam.rotate_speed,
        .min_dist = _lopgl.orbital_cam.min_dist,
        .max_dist = _lopgl.orbital_cam.max_dist,
        .min_pitch = _lopgl.orbital_cam.min_pitch,
        .max_pitch = _lopgl.orbital_cam.max_pitch
    };
}

lopgl_fp_cam_desc_t lopgl_get_fp_cam_desc() {
    return (lopgl_fp_cam_desc_t) {
        .position = _lopgl.fp_cam.position,
        .world_up = _lopgl.fp_cam.world_up,
        .yaw = _lopgl.fp_cam.yaw,
        .pitch = _lopgl.fp_cam.pitch,
        .zoom = _lopgl.fp_cam.zoom,
        .movement_speed = _lopgl.fp_cam.movement_speed,
        .aim_speed = _lopgl.fp_cam.aim_speed,
        .zoom_speed = _lopgl.fp_cam.zoom_speed,
        .min_pitch = _lopgl.fp_cam.min_pitch,
        .max_pitch = _lopgl.fp_cam.max_pitch,
        .min_zoom = _lopgl.fp_cam.min_zoom,
        .max_zoom = _lopgl.fp_cam.max_zoom
    };
}

void lopgl_set_orbital_cam(lopgl_orbital_cam_desc_t* desc) {
    // camera attributes
    _lopgl.orbital_cam.target = desc->target;
    _lopgl.orbital_cam.up = desc->up;
    _lopgl.orbital_cam.polar = HMM_Vec2(desc->pitch, desc->heading);
    _lopgl.orbital_cam.distance = desc->distance;
    // limits
    _lopgl.orbital_cam.min_pitch = desc->min_pitch;
	_lopgl.orbital_cam.max_pitch = desc->max_pitch;
	_lopgl.orbital_cam.min_dist = desc->min_dist;
	_lopgl.orbital_cam.max_dist = desc->max_dist;
    // control options
    _lopgl.orbital_cam.rotate_speed = desc->rotate_speed;
    _lopgl.orbital_cam.zoom_speed = desc->zoom_speed;
    // control state
    _lopgl.orbital_cam.enable_rotate = false;

    update_orbital_cam_vectors(&_lopgl.orbital_cam);
}

void lopgl_set_fp_cam(lopgl_fp_cam_desc_t* desc) {
    // camera attributes
    _lopgl.fp_cam.position = desc->position;
    _lopgl.fp_cam.world_up = desc->world_up;
    _lopgl.fp_cam.yaw = desc->yaw;
    _lopgl.fp_cam.pitch = desc->pitch;
    _lopgl.fp_cam.zoom = desc->zoom;
    // limits
    _lopgl.fp_cam.min_pitch = desc->min_pitch;
	_lopgl.fp_cam.max_pitch = desc->max_pitch;
	_lopgl.fp_cam.min_zoom = desc->min_zoom;
	_lopgl.fp_cam.max_zoom = desc->max_zoom;
    // control options
    _lopgl.fp_cam.movement_speed = desc->movement_speed;
    _lopgl.fp_cam.aim_speed = desc->aim_speed;
    _lopgl.fp_cam.zoom_speed = desc->zoom_speed;
    // control state
    _lopgl.fp_cam.enable_aim = false;
    _lopgl.fp_cam.move_forward = false;
    _lopgl.fp_cam.move_backward = false;
    _lopgl.fp_cam.move_left = false;
    _lopgl.fp_cam.move_right = false;

    update_fp_cam_vectors(&_lopgl.fp_cam);
}

hmm_mat4 lopgl_view_matrix() {
    if (_lopgl.fp_enabled) {
        return view_matrix_fp(&_lopgl.fp_cam);
    }
    else {
        return view_matrix_orbital(&_lopgl.orbital_cam);
    }
}

float lopgl_fov() {
    return 45.0f;
}

hmm_vec3 lopgl_camera_position() {
    if (_lopgl.fp_enabled) {
        return _lopgl.fp_cam.position;
    }
    else {
        return _lopgl.orbital_cam.position;
    }
}

hmm_vec3 lopgl_camera_direction() {
    if (_lopgl.fp_enabled) {
        return _lopgl.fp_cam.front;
    }
    else {
        return HMM_NormalizeVec3(HMM_SubtractVec3(_lopgl.orbital_cam.target, _lopgl.orbital_cam.position));
    }
}

void lopgl_handle_input(const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_C) {
            _lopgl.fp_enabled = !_lopgl.fp_enabled;
        }
        else if (e->key_code == SAPP_KEYCODE_H) {
            _lopgl.show_help = !_lopgl.show_help;
        }
        else if (e->key_code == SAPP_KEYCODE_U) {
            _lopgl.hide_ui = !_lopgl.hide_ui;
        }
        else if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }

    hmm_vec2 mouse_offset = HMM_Vec2(0.0f, 0.0f);

    if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (!_lopgl.first_mouse) {
            mouse_offset.X = e->mouse_x - _lopgl.last_mouse.X;
            mouse_offset.Y = _lopgl.last_mouse.Y - e->mouse_y;
        }
        else {
            _lopgl.first_mouse = false;
        }

        _lopgl.last_mouse.X = e->mouse_x;
        _lopgl.last_mouse.Y = e->mouse_y;
    }

    if (_lopgl.fp_enabled) {
        handle_input_fp(&_lopgl.fp_cam, e, mouse_offset);
    }
    else {
        handle_input_orbital(&_lopgl.orbital_cam, e, mouse_offset);
    }
}

bool lopgl_ui_visible() {
    return !_lopgl.hide_ui;
}

void lopgl_render_help() {
    if (_lopgl.hide_ui) {
        return;
    }

    sdtx_canvas(sapp_width()*0.5f, sapp_height()*0.5f);
    sdtx_origin(0.25f, 0.25f);
    sdtx_home();

    if (!_lopgl.show_help) {
        sdtx_color4b(0xff, 0xff, 0xff, 0xaf);
        sdtx_puts(  "Show help:\t'H'");
    }
    else {
        sdtx_color4b(0x00, 0xff, 0x00, 0xaf);
        sdtx_puts(  "Hide help:\t'H'\n\n");
        sdtx_printf("Frame Time:\t%.3f\n\n", stm_ms(_lopgl.frame_time));
        sdtx_printf("Orbital Cam\t[%c]\n", _lopgl.fp_enabled ? ' ': '*');
        sdtx_printf("FP Cam\t\t[%c]\n\n", _lopgl.fp_enabled ? '*' : ' ');
        sdtx_puts("Switch Cam:\t'C'\n\n");

        if (_lopgl.fp_enabled) {
            sdtx_puts(help_fp(&_lopgl.fp_cam));
        }
        else {
            sdtx_puts(help_orbital(&_lopgl.orbital_cam));
        }

        sdtx_puts("\nExit:\t\t'ESC'");
    }
    
    sdtx_draw();
}

void lopgl_render_gles2_fallback(void) {
    const sg_pass_action pass_action = {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 1.0f, 0.0f, 0.0f, 1.0f } },
    };
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    
    sdtx_canvas(sapp_width()*0.5f, sapp_height()*0.5f);
    sdtx_origin(0.25f, 0.25f);
    sdtx_home();
    sdtx_color4b(0xff, 0xff, 0xff, 0xff);
    sdtx_puts("This browser does not support WebGL 2.\n");
    sdtx_puts("Try Chrome, Edge or Firefox.");
    sdtx_draw();

    sg_end_pass();
    sg_commit();
}

typedef struct {
    sg_image img_id;
    sg_wrap wrap_u;
    sg_wrap wrap_v;
    lopgl_fail_callback_t fail_callback;
} lopgl_img_request_data;

/* The fetch-callback is called by sokol_fetch.h when the data is loaded,
   or when an error has occurred.
*/
static void image_fetch_callback(const sfetch_response_t* response) {
    lopgl_img_request_data req_data = *(lopgl_img_request_data*)response->user_data;

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
            /* initialize the sokol-gfx texture */
            sg_init_image(req_data.img_id, &(sg_image_desc){
                .width = img_width,
                .height = img_height,
                /* set pixel_format to RGBA8 for WebGL */
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .wrap_u = req_data.wrap_u,
                .wrap_v = req_data.wrap_v,
                .min_filter = SG_FILTER_LINEAR,
                .mag_filter = SG_FILTER_LINEAR,
                .content.subimage[0][0] = {
                    .ptr = pixels,
                    .size = img_width * img_height * desired_channels,
                }
            });
            stbi_image_free(pixels);
        }
    }
    else if (response->failed) {
        req_data.fail_callback();
    }
}

typedef struct {
    fastObjMesh* mesh;
    lopgl_obj_request_callback_t callback;
    lopgl_fail_callback_t fail_callback;
    void* buffer_ptr;
    uint32_t buffer_size;
    void* user_data_ptr;
} lopgl_obj_request_data;

static void mtl_fetch_callback(const sfetch_response_t* response) {
    lopgl_obj_request_data req_data = *(lopgl_obj_request_data*)response->user_data;

    if (response->fetched) {
        fast_obj_mtllib_read(req_data.mesh, response->buffer_ptr, response->fetched_size);
        req_data.callback(&(lopgl_obj_response_t){
            .mesh = req_data.mesh,
            .user_data_ptr = req_data.user_data_ptr
        });
    }
    else if (response->failed) {
        req_data.fail_callback();
    }

    fast_obj_destroy(req_data.mesh);
}

static void obj_fetch_callback(const sfetch_response_t* response) {
    lopgl_obj_request_data req_data = *(lopgl_obj_request_data*)response->user_data;

    if (response->fetched) {
        /* the file data has been fetched, since we provided a big-enough
           buffer we can be sure that all data has been loaded here
        */
        req_data.mesh = fast_obj_read(response->buffer_ptr, response->fetched_size);

        for (unsigned int i = 0; i < req_data.mesh->mtllib_count; ++i) {
            sfetch_send(&(sfetch_request_t){
                .path = req_data.mesh->mtllibs[i],
                .callback = mtl_fetch_callback,
                .buffer_ptr = req_data.buffer_ptr,
                .buffer_size = req_data.buffer_size,
                .user_data_ptr = &req_data,
                .user_data_size = sizeof(req_data)
            });
        }
    }
    else if (response->failed) {
        req_data.fail_callback();
    }
}

void lopgl_load_image(const lopgl_image_request_t* request) {
    lopgl_img_request_data req_data = {
        .img_id = request->img_id,
        .wrap_u = request->wrap_u,
        .wrap_v = request->wrap_v,
        .fail_callback = request->fail_callback
    };

    sfetch_send(&(sfetch_request_t){
        .path = request->path,
        .callback = image_fetch_callback,
        .buffer_ptr = request->buffer_ptr,
        .buffer_size = request->buffer_size,
        .user_data_ptr = &req_data,
        .user_data_size = sizeof(req_data)
    });
}

void lopgl_load_obj(const lopgl_obj_request_t* request) {
    lopgl_obj_request_data req_data = {
        .mesh = 0,
        .callback = request->callback,
        .fail_callback = request->fail_callback,
        .buffer_ptr = request->buffer_ptr,
        .buffer_size = request->buffer_size,
        .user_data_ptr = request->user_data_ptr
    };

    sfetch_send(&(sfetch_request_t){
        .path = request->path,
        .callback = obj_fetch_callback,
        .buffer_ptr = request->buffer_ptr,
        .buffer_size = request->buffer_size,
        .user_data_ptr = &req_data,
        .user_data_size = sizeof(req_data)
    });
}

/*=== LOAD CUBEMAP IMPLEMENTATION ==================================================*/

typedef struct _cubemap_request_instance_t {
    int index;
    _cubemap_request_t* request;
} _cubemap_request_instance_t;

static bool load_cubemap(_cubemap_request_t* request) {
    const int desired_channels = 4;
    int img_widths[6], img_heights[6];
    stbi_uc* pixels_ptrs[6];
    sg_image_content img_content;

    for (int i = 0; i < 6; ++i) {
        int num_channel;
        pixels_ptrs[i] = stbi_load_from_memory(
            request->buffer + (i * request->buffer_offset),
            request->fetched_sizes[i],
            &img_widths[i], &img_heights[i],
            &num_channel, desired_channels);    

        img_content.subimage[i][0].ptr = pixels_ptrs[i];
        img_content.subimage[i][0].size = img_widths[i] * img_heights[i] * desired_channels;
    }

    bool valid = img_widths[0] > 0 && img_heights[0] > 0;

    for (int i = 1; i < 6; ++i) {
        if (img_widths[i] != img_widths[0] || img_heights[i] != img_heights[0]) {
            valid = false;
            break;
        }
    }
    
    if (valid) {
        /* initialize the sokol-gfx texture */
        sg_init_image(request->img_id, &(sg_image_desc){
            .type = SG_IMAGETYPE_CUBE,
            .width = img_widths[0],
            .height = img_heights[0],
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
    
    return valid;
}

static void cubemap_fetch_callback(const sfetch_response_t* response) {
    _cubemap_request_instance_t req_inst = *(_cubemap_request_instance_t*)response->user_data;
    _cubemap_request_t* request = req_inst.request;

    if (response->fetched) {
        request->fetched_sizes[req_inst.index] = response->fetched_size;
        ++request->finished_requests;
    }
    else if (response->failed) {
        request->failed = true;
        ++request->finished_requests;
    }

    if (request->finished_requests == 6) {
        if (!request->failed) {
            request->failed = !load_cubemap(request);
        }
        
        if (request->failed) {
            request->fail_callback();
        }
    }
}

void lopgl_load_cubemap(lopgl_cubemap_request_t* request) {
    // TODO: cleanup and limit cubemap requests
    _lopgl.cubemap_req = (_cubemap_request_t) {
        .img_id = request->img_id,
        .buffer = request->buffer_ptr,
        .buffer_offset = request->buffer_offset,
        .fail_callback = request->fail_callback
    };

    const char* cubemap[6] = {
        request->path_right,
        request->path_left,
        request->path_top,
        request->path_bottom,
        request->path_front,
        request->path_back
    };

    for (int i = 0; i < 6; ++i) {
        _cubemap_request_instance_t req_instance = {
            .index = i,
            .request = &_lopgl.cubemap_req
        };
        sfetch_send(&(sfetch_request_t){
            .path = cubemap[i],
            .callback = cubemap_fetch_callback,
            .buffer_ptr = request->buffer_ptr + (i * request->buffer_offset),
            .buffer_size = request->buffer_offset,
            .user_data_ptr = &req_instance,
            .user_data_size = sizeof(req_instance)
        });
    }
}

/*=== ORBITAL CAM IMPLEMENTATION ==================================================*/

static void update_orbital_cam_vectors(struct orbital_cam* camera) {
    float cos_p = cosf(HMM_ToRadians(camera->polar.X));
    float sin_p = sinf(HMM_ToRadians(camera->polar.X));
    float cos_h = cosf(HMM_ToRadians(camera->polar.Y));
    float sin_h = sinf(HMM_ToRadians(camera->polar.Y));
    camera->position = HMM_Vec3(
        camera->distance * cos_p * sin_h,
        camera->distance * -sin_p,
        camera->distance * cos_p * cos_h
    );
}

hmm_mat4 view_matrix_orbital(struct orbital_cam* camera) {
    return HMM_LookAt(camera->position, camera->target, camera->up);
}

static void move_orbital_camera(struct orbital_cam* camera, hmm_vec2 mouse_offset) {
    camera->polar.Y -= mouse_offset.X * camera->rotate_speed;
	const float pitch = camera->polar.X + mouse_offset.Y * camera->rotate_speed;
	camera->polar.X = HMM_Clamp(camera->min_pitch, pitch, camera->max_pitch);
}

static void zoom_orbital_camera(struct orbital_cam* camera, float val) {
    const float new_dist = camera->distance - val * camera->zoom_speed;
	camera->distance = HMM_Clamp(new_dist, camera->min_dist, camera->max_dist);
}

void handle_input_orbital(struct orbital_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset) {

    if (e->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
		if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			camera->enable_rotate = true;
		}
	}
	else if (e->type == SAPP_EVENTTYPE_MOUSE_UP) {
		if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			camera->enable_rotate = false;
		}
	}
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (camera->enable_rotate) {
            move_orbital_camera(camera, mouse_offset);
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        zoom_orbital_camera(camera, e->scroll_y);
    }
    else if (e->type == SAPP_EVENTTYPE_TOUCHES_BEGAN) {
        for (int i = 0; i < e->num_touches; ++i) {
            const sapp_touchpoint* touch = &e->touches[i];
            _lopgl.last_touch[touch->identifier].X = touch->pos_x;
            _lopgl.last_touch[touch->identifier].Y = touch->pos_y;
        }
    }
    else if (e->type == SAPP_EVENTTYPE_TOUCHES_MOVED) {
        if (e->num_touches == 1) {
            const sapp_touchpoint* touch = &e->touches[0];
            hmm_vec2* last_touch = &_lopgl.last_touch[touch->identifier];

            hmm_vec2 mouse_offset = HMM_Vec2(0.0f, 0.0f);

            mouse_offset.X = touch->pos_x - last_touch->X;
            mouse_offset.Y = last_touch->Y - touch->pos_y;

            // reduce speed of touch controls
            mouse_offset.X *= 0.3;
            mouse_offset.Y *= 0.3;

            move_orbital_camera(camera, mouse_offset);
        }
        else if(e->num_touches == 2) {
            const sapp_touchpoint* touch0 = &e->touches[0];
            const sapp_touchpoint* touch1 = &e->touches[1];

            const hmm_vec2 v0 = HMM_Vec2(touch0->pos_x, touch0->pos_y);
            const hmm_vec2 v1 = HMM_Vec2(touch1->pos_x, touch1->pos_y);

            const hmm_vec2* prev_v0 = &_lopgl.last_touch[touch0->identifier];
            const hmm_vec2* prev_v1 = &_lopgl.last_touch[touch1->identifier];

            const float length0 = HMM_LengthVec2(HMM_SubtractVec2(v1, v0));
            const float length1 = HMM_LengthVec2(HMM_SubtractVec2(*prev_v1, *prev_v0));

            float diff = length0 - length1;
            // reduce speed of touch controls
            diff *= 0.1;

            zoom_orbital_camera(camera, diff);
        }

        // update all touch coords
        for (int i = 0; i < e->num_touches; ++i) {
            const sapp_touchpoint* touch = &e->touches[i];
            _lopgl.last_touch[touch->identifier].X = touch->pos_x;
            _lopgl.last_touch[touch->identifier].Y = touch->pos_y;
        }
    }

    update_orbital_cam_vectors(camera);
}

const char* help_orbital() {
    return  "Look:\t\tleft-mouse-btn\n"
            "Zoom:\t\tmouse-scroll\n";
}

/*=== FP CAM IMPLEMENTATION =======================================================*/

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum camera_movement {
    CAM_MOV_FORWARD,
    CAM_MOV_BACKWARD,
    CAM_MOV_LEFT,
    CAM_MOV_RIGHT
};

static void update_fp_cam_vectors(struct fp_cam* camera) {
    // Calculate the new Front vector
    hmm_vec3 front;
    front.X = cosf(HMM_ToRadians(camera->yaw)) * cosf(HMM_ToRadians(camera->pitch));
    front.Y = sinf(HMM_ToRadians(camera->pitch));
    front.Z = sinf(HMM_ToRadians(camera->yaw)) * cosf(HMM_ToRadians(camera->pitch));
    camera->front = HMM_NormalizeVec3(front);
    // Also re-calculate the Right and Up vector
    // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    camera->right = HMM_NormalizeVec3(HMM_Cross(camera->front, camera->world_up));  
    camera->up    = HMM_NormalizeVec3(HMM_Cross(camera->right, camera->front));
}

hmm_mat4 view_matrix_fp(struct fp_cam* camera) {
    hmm_vec3 target = HMM_AddVec3(camera->position, camera->front);
    return HMM_LookAt(camera->position, target, camera->up);
}

void update_fp_camera(struct fp_cam* camera, float delta_time) {
    float velocity = camera->movement_speed * delta_time;
    if (camera->move_forward) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->front, velocity);
        camera->position = HMM_AddVec3(camera->position, offset);
    }
    if (camera->move_backward) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->front, velocity);
        camera->position = HMM_SubtractVec3(camera->position, offset);
    }
    if (camera->move_left) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->right, velocity);
        camera->position = HMM_SubtractVec3(camera->position, offset);
    }
    if (camera->move_right) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->right, velocity);
        camera->position = HMM_AddVec3(camera->position, offset);
    }
}

static void aim_fp_camera(struct fp_cam* camera, hmm_vec2 mouse_offset) {
    camera->yaw   += mouse_offset.X * camera->aim_speed;
    camera->pitch += mouse_offset.Y * camera->aim_speed;

    camera->pitch = HMM_Clamp(camera->min_pitch, camera->pitch, camera->max_pitch);

    update_fp_cam_vectors(camera);
}

static void zoom_fp_camera(struct fp_cam* camera, float yoffset) {
    camera->zoom -= yoffset * camera->zoom_speed;
    camera->zoom = HMM_Clamp(camera->min_zoom, camera->zoom, camera->max_zoom);
}

void handle_input_fp(struct fp_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_W || e->key_code == SAPP_KEYCODE_UP) {
            camera->move_forward = true;
        }
        else if (e->key_code == SAPP_KEYCODE_S || e->key_code == SAPP_KEYCODE_DOWN) {
            camera->move_backward = true;
        }
        else if (e->key_code == SAPP_KEYCODE_A || e->key_code == SAPP_KEYCODE_LEFT) {
            camera->move_left = true;
        }
        else if (e->key_code == SAPP_KEYCODE_D || e->key_code == SAPP_KEYCODE_RIGHT) {
            camera->move_right = true;
        }
    }
    else if (e->type == SAPP_EVENTTYPE_KEY_UP) {
        if (e->key_code == SAPP_KEYCODE_W || e->key_code == SAPP_KEYCODE_UP) {
            camera->move_forward = false;
        }
        else if (e->key_code == SAPP_KEYCODE_S || e->key_code == SAPP_KEYCODE_DOWN) {
            camera->move_backward = false;
        }
        else if (e->key_code == SAPP_KEYCODE_A || e->key_code == SAPP_KEYCODE_LEFT) {
            camera->move_left = false;
        }
        else if (e->key_code == SAPP_KEYCODE_D || e->key_code == SAPP_KEYCODE_RIGHT) {
            camera->move_right = false;
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
		if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			camera->enable_aim = true;
		}
	}
	else if (e->type == SAPP_EVENTTYPE_MOUSE_UP) {
		if (e->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			camera->enable_aim = false;
		}
	}
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (camera->enable_aim) {
            aim_fp_camera(camera, mouse_offset);
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        zoom_fp_camera(camera, e->scroll_y);
    }
}

const char* help_fp() {
    return  "Forward:\t'W' '\xf0'\n"
            "Left:\t\t'A' '\xf2\'\n"
            "Back:\t\t'S' '\xf1\'\n"
            "Right:\t\t'D' '\xf3\'\n"
            "Look:\t\tleft-mouse-btn\n"
            "Zoom:\t\tmouse-scroll\n";
}

#endif /*LOPGL_APP_IMPL*/
