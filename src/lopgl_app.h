#ifndef LOPGL_APP_INCLUDED
#define LOPGL_APP_INCLUDED

#include "sokol_app.h"
#include <hmm/HandmadeMath.h>

void lopgl_setup();

void lopgl_update();

void lopgl_shutdown();

hmm_mat4 lopgl_get_view_matrix();

void lopgl_handle_input(const sapp_event* e);

void lopgl_render_help();

#endif /*LOPGL_APP_INCLUDED*/


/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef LOPGL_APP_IMPL

#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#define SOKOL_DEBUGTEXT_IMPL
#include "sokol_debugtext.h"
#include <stdbool.h>

/*=== ORBITAL CAM ==================================================*/

struct orbital_cam {
    // camera attributes
    hmm_vec3 target;
    hmm_vec3 up;
    hmm_vec3 position;
    hmm_vec3 world_up;
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
    // control state
    bool enable_rotate;
};

struct orbital_cam create_orbital_camera(hmm_vec3 target, hmm_vec3 up, float pitch, float heading, float distance);

hmm_mat4 get_view_matrix_orbital(struct orbital_cam* camera);

void handle_input_orbital(struct orbital_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset);

const char* get_help_orbital();

/*=== FP CAM =======================================================*/

struct fp_cam {
    // camera attributes
    hmm_vec3 position;
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 right;
    hmm_vec3 world_up;
    float zoom;
    float yaw;
    float pitch;
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
};

struct fp_cam create_fp_camera(hmm_vec3 position, hmm_vec3 up, float yaw, float pitch);

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
hmm_mat4 get_view_matrix_fp(struct fp_cam* camera);

void handle_input_fp(struct fp_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset);

void update_fp_camera(struct fp_cam* camera, float delta_time);

const char* get_help_fp();

/*=== APP ==========================================================*/

typedef struct {
    struct orbital_cam orbital_cam;
    struct fp_cam fp_cam;
    bool fp_enabled;
    bool show_help;
    bool first_mouse;
    float last_x;
    float last_y;
    uint64_t time_stamp;
    uint64_t frame_time;
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

    _lopgl.orbital_cam = create_orbital_camera(HMM_Vec3(0.0f, 0.0f,  0.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), 0.0f, 0.0f, 6.0f);
    _lopgl.fp_cam = create_fp_camera(HMM_Vec3(0.0f, 0.0f,  5.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), -90.f, 0.0f);
    _lopgl.fp_enabled = false;
    _lopgl.first_mouse = true;
    _lopgl.show_help = false;
}

void lopgl_update() {
    _lopgl.frame_time = stm_laptime(&_lopgl.time_stamp);
    
    if (_lopgl.fp_enabled) {
        update_fp_camera(&_lopgl.fp_cam, stm_ms(_lopgl.frame_time));
    }
}

void lopgl_shutdown() {
    sg_shutdown();
}

hmm_mat4 lopgl_get_view_matrix() {
    if (_lopgl.fp_enabled) {
        return get_view_matrix_fp(&_lopgl.fp_cam);
    }
    else {
        return get_view_matrix_orbital(&_lopgl.orbital_cam);
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
        else if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
        }
    }

    hmm_vec2 mouse_offset = HMM_Vec2(0.0f, 0.0f);

    if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (!_lopgl.first_mouse) {
            mouse_offset.X = e->mouse_x - _lopgl.last_x;
            mouse_offset.Y = _lopgl.last_y - e->mouse_y;
        }
        else {
            _lopgl.first_mouse = false;
        }

        _lopgl.last_x = e->mouse_x;
        _lopgl.last_y = e->mouse_y;
    }

    if (_lopgl.fp_enabled) {
        handle_input_fp(&_lopgl.fp_cam, e, mouse_offset);
    }
    else {
        handle_input_orbital(&_lopgl.orbital_cam, e, mouse_offset);
    }
}

void lopgl_render_help() {
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

        if (_lopgl.fp_enabled) {
            sdtx_puts(  "Orbital Cam\t[ ]\n"
                        "FP Cam\t\t[*]\n\n");
        }
        else {
            sdtx_puts(  "Orbital Cam\t[*]\n"
                        "FP Cam\t\t[ ]\n\n");
        }

        sdtx_puts("Switch Cam:\t'C'\n\n");

        if (_lopgl.fp_enabled) {
            sdtx_puts(get_help_fp(&_lopgl.fp_cam));
        }
        else {
            sdtx_puts(get_help_orbital(&_lopgl.orbital_cam));
        }

        sdtx_puts("\nExit:\t\t'ESC'");
    }
    
    sdtx_draw();
}

/*=== ORBITAL CAM IMPLEMENTATION ==================================================*/

// Default camera values
static const float MIN_PITCH_ORB = -89;
static const float MAX_PITCH_ORB = 89;
static const float MIN_DISTANCE = 1;
static const float MAX_DISTANCE = 10;
static const float ROTATE_SPEED = 1.0f;
static const float ZOOM_SPEED = 0.5f;

static void update_camera_vectors(struct orbital_cam* camera) {
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

struct orbital_cam create_orbital_camera(hmm_vec3 target, hmm_vec3 up, float pitch, float heading, float distance) {
    struct orbital_cam camera;
    // camera attributes
    camera.target = target;
    camera.up = up;
    camera.polar = HMM_Vec2(pitch, heading);
    camera.distance = distance;
    // limits
    camera.min_pitch = MIN_PITCH_ORB;
	camera.max_pitch = MAX_PITCH_ORB;
	camera.min_dist = MIN_DISTANCE;
	camera.max_dist = MAX_DISTANCE;
    // control options
    camera.rotate_speed = ROTATE_SPEED;
    camera.zoom_speed = ZOOM_SPEED;
    // control state
    camera.enable_rotate = false;

    update_camera_vectors(&camera);
    
    return camera;
}

hmm_mat4 get_view_matrix_orbital(struct orbital_cam* camera) {
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

    update_camera_vectors(camera);
}

const char* get_help_orbital() {
    return  "Look:\t\tleft-mouse-btn\n"
            "Zoom:\t\tmouse-scroll\n";
}

/*=== FP CAM IMPLEMENTATION =======================================================*/

// Default camera values
static const float ZOOM             =  45.0f;
static const float MIN_PITCH        = -89.0f;
static const float MAX_PITCH        =  89.0f;
static const float MIN_ZOOM         =  1.0f;
static const float MAX_ZOOM         =  45.0f;
static const float MOVEMENT_SPEED   =  0.005f;
static const float AIM_SPEED        =  1.0f;
static const float ZOOM_SPEED_FP    =  0.1f;

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum camera_movement {
    CAM_MOV_FORWARD,
    CAM_MOV_BACKWARD,
    CAM_MOV_LEFT,
    CAM_MOV_RIGHT
};

static void update_fp_camera_vectors(struct fp_cam* camera) {
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

struct fp_cam create_fp_camera(hmm_vec3 position, hmm_vec3 up, float yaw, float pitch) {
    struct fp_cam camera;
    // camera attributes
    camera.position = position;
    camera.world_up = up;
    camera.yaw = yaw;
    camera.pitch = pitch;
    camera.zoom = ZOOM;
    // limits
    camera.min_pitch = MIN_PITCH;
	camera.max_pitch = MAX_PITCH;
	camera.min_zoom = MIN_ZOOM;
	camera.max_zoom = MAX_ZOOM;
    // control options
    camera.movement_speed = MOVEMENT_SPEED;
    camera.aim_speed = AIM_SPEED;
    camera.zoom_speed = ZOOM_SPEED_FP;
    // control state
    camera.enable_aim = false;
    camera.move_forward = false;
    camera.move_backward = false;
    camera.move_left = false;
    camera.move_right = false;

    update_fp_camera_vectors(&camera);
    
    return camera;
}

hmm_mat4 get_view_matrix_fp(struct fp_cam* camera) {
    hmm_vec3 direction = HMM_AddVec3(camera->position, camera->front);
    return HMM_LookAt(camera->position, direction, camera->up);
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

    update_fp_camera_vectors(camera);
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

const char* get_help_fp() {
    return  "Forward:\t'W' '\xf0'\n"
            "Left:\t\t'A' '\xf2\'\n"
            "Back:\t\t'S' '\xf1\'\n"
            "Right:\t\t'D' '\xf3\'\n"
            "Look:\t\tleft-mouse-btn\n"
            "Zoom:\t\tmouse-scroll\n";
}

#endif /*LOPGL_APP_IMPL*/
