#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include <hmm/HandmadeMath.h>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum camera_movement {
    CAM_MOV_FORWARD,
    CAM_MOV_BACKWARD,
    CAM_MOV_LEFT,
    CAM_MOV_RIGHT
};

// Default camera values
static const float YAW         = -90.0f;
static const float PITCH       =  0.0f;
static const float SPEED       =  2.5f;
static const float SENSITIVITY =  0.1f;
static const float ZOOM        =  45.0f;

struct cam_desc {
    // camera attributes
    hmm_vec3 position;
    hmm_vec3 front;
    hmm_vec3 up;
    hmm_vec3 right;
    hmm_vec3 world_up;
    // euler angles
    float yaw;
    float pitch;
    // camera options
    float movement_speed;
    float mouse_sensitivity;
    float zoom;
    bool movement_enabled;
};

struct cam_desc create_camera(hmm_vec3 position, hmm_vec3 up, float yaw, float pitch);

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
hmm_mat4 get_view_matrix(const struct cam_desc* camera);

// Enable or disable camera movement
void toggle_camera_movement(struct cam_desc* camera);

 // Processes input received from any keyboard-like input system.
 // Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void process_keyboard(struct cam_desc* camera, enum camera_movement direction, float delta_time);

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void process_mouse_movement(struct cam_desc* camera, float xoffset, float yoffset);

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void process_mouse_scroll(struct cam_desc* camera, float yoffset);

#endif