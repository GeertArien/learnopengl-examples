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
    bool show_help;
    uint64_t time_stamp;
    uint64_t frame_time;
};

struct camera create_camera();

void update(struct camera* camera);

hmm_mat4 get_view_matrix(struct camera* camera);

void handle_input(struct camera* camera, const sapp_event* e);

void render_help();

#endif
