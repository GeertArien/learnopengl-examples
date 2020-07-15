#ifndef FP_CAMERA_H
#define FP_CAMERA_H

#include <stdbool.h>
#include <sokol_app.h>
#include <hmm/HandmadeMath.h>

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
};

struct fp_cam create_fp_camera(hmm_vec3 position, hmm_vec3 up, float yaw, float pitch);

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
hmm_mat4 get_view_matrix_fp(struct fp_cam* camera);

void handle_input_fp(struct fp_cam* camera, const sapp_event* e, hmm_vec2 mouse_offset, float delta_time);

const char* get_help_fp();

#endif