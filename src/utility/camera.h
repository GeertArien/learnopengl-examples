#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include "fp_camera.h"
#include "orbital_camera.h"

struct camera {
    struct orbital_cam orbital_cam;
    struct fp_cam fp_cam;
    bool fp_enabled;
    bool mouse_shown;
    bool first_mouse;
    float last_x;
    float last_y;
};

struct camera create_camera();

hmm_mat4 get_view_matrix(struct camera* camera);

void handle_input(struct camera* camera, const sapp_event* e, float delta_time);

#endif
