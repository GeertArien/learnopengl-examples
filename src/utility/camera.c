#include "camera.h"

struct camera create_camera() {
    struct camera camera;
    
    camera.orbital_cam = create_orbital_camera(HMM_Vec3(0.0f, 0.0f,  0.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), 0.0f, 0.0f, 5.0f);
    camera.fp_cam = create_fp_camera(HMM_Vec3(0.0f, 0.0f,  3.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), -90.f, 0.0f);
    camera.fp_enabled = false;
    camera.mouse_shown = true;
    camera.first_mouse = true;
    
    return camera;
}

hmm_mat4 get_view_matrix(struct camera* camera) {
    if (camera->fp_enabled) {
        return get_view_matrix_fp(&camera->fp_cam);
    }
    else {
        return get_view_matrix_orbital(&camera->orbital_cam);
    }
}

void handle_input(struct camera* camera, const sapp_event* e, float delta_time) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_C) {
            camera->fp_enabled = !camera->fp_enabled;
        }
        else if (e->key_code == SAPP_KEYCODE_SPACE) {
            bool mouse_shown = sapp_mouse_shown();
            sapp_show_mouse(!mouse_shown);
        }
    }

    hmm_vec2 mouse_offset = HMM_Vec2(0.0f, 0.0f);

    if (e->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
        if (!camera->first_mouse) {
            mouse_offset.X = e->mouse_x - camera->last_x;
            mouse_offset.Y = camera->last_y - e->mouse_y;
        }
        else {
            camera->first_mouse = false;
        }

        camera->last_x = e->mouse_x;
        camera->last_y = e->mouse_y;
    }

    if (camera->fp_enabled) {
        handle_input_fp(&camera->fp_cam, e, mouse_offset, delta_time);
    }
    else {
        handle_input_orbital(&camera->orbital_cam, e, mouse_offset);
    }
}
