#ifndef ORBITAL_CAMERA_H
#define ORBITAL_CAMERA_H

#include <stdbool.h>
#include <sokol_app.h>
#include <hmm/HandmadeMath.h>

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

#endif