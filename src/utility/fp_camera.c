#include "fp_camera.h"

// Default camera values
static const float YAW              = -90.0f;
static const float PITCH            =  0.0f;
static const float ZOOM             =  45.0f;
static const float MIN_PITCH        = -89.0f;
static const float MAX_PITCH        =  89.0f;
static const float MIN_ZOOM         =  1.0f;
static const float MAX_ZOOM         =  45.0f;
static const float MOVEMENT_SPEED   =  2.5f;
static const float AIM_SPEED        =  1.0f;
static const float ZOOM_SPEED       =  0.1f;

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum camera_movement {
    CAM_MOV_FORWARD,
    CAM_MOV_BACKWARD,
    CAM_MOV_LEFT,
    CAM_MOV_RIGHT
};

static void update_camera_vectors(struct fp_cam* camera) {
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
    camera.zoom_speed = ZOOM_SPEED;
    // control state
    camera.movement_enabled = true;

    update_camera_vectors(&camera);
    
    return camera;
}

hmm_mat4 get_view_matrix_fp(struct fp_cam* camera) {
    hmm_vec3 direction = HMM_AddVec3(camera->position, camera->front);
    return HMM_LookAt(camera->position, direction, camera->up);
}

static void toggle_fp_camera_movement(struct fp_cam* camera) {
    camera->movement_enabled = !camera->movement_enabled;
}

static void move_fp_camera(struct fp_cam* camera, enum camera_movement direction, float delta_time) {
    if (!camera->movement_enabled) {
        return;
    }

    float velocity = camera->movement_speed * delta_time;
    if (direction == CAM_MOV_FORWARD) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->front, velocity);
        camera->position = HMM_AddVec3(camera->position, offset);
    }
    else if (direction == CAM_MOV_BACKWARD) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->front, velocity);
        camera->position = HMM_SubtractVec3(camera->position, offset);
    }
    else if (direction == CAM_MOV_LEFT) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->right, velocity);
        camera->position = HMM_SubtractVec3(camera->position, offset);
    }
    else if (direction == CAM_MOV_RIGHT) {
        hmm_vec3 offset = HMM_MultiplyVec3f(camera->right, velocity);
        camera->position = HMM_AddVec3(camera->position, offset);
    }
}

static void aim_fp_camera(struct fp_cam* camera, hmm_vec2 mouse_offset) {
    if (!camera->movement_enabled) {
        return;
    }

    camera->yaw   += mouse_offset.X * camera->aim_speed;
    camera->pitch += mouse_offset.Y * camera->aim_speed;

    camera->pitch = HMM_Clamp(camera->min_pitch, camera->pitch, camera->max_pitch);

    update_camera_vectors(camera);
}

static void zoom_fp_camera(struct fp_cam* camera, float yoffset) {
    if (!camera->movement_enabled) {
        return;
    }
     
    camera->zoom -= yoffset * camera->zoom_speed;
    camera->zoom = HMM_Clamp(camera->min_zoom, camera->zoom, camera->max_zoom);
}

void handle_input_fp(struct fp_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset, float delta_time) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_W) {
            move_fp_camera(camera, CAM_MOV_FORWARD, delta_time);
        }
        else if (e->key_code == SAPP_KEYCODE_S) {
            move_fp_camera(camera, CAM_MOV_BACKWARD, delta_time);
        }
        else if (e->key_code == SAPP_KEYCODE_A) {
            move_fp_camera(camera, CAM_MOV_LEFT, delta_time);
        }
        else if (e->key_code == SAPP_KEYCODE_D) {
            move_fp_camera(camera, CAM_MOV_RIGHT, delta_time);
        }
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        aim_fp_camera(camera, mouse_offset);
    }
    else if (e->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
        zoom_fp_camera(camera, e->scroll_y);
    }
}
