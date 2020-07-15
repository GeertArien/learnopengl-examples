#include "camera.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include "sokol_gfx.h"
#include "sokol_debugtext.h"

struct camera create_camera() {
    struct camera camera;
    
    camera.orbital_cam = create_orbital_camera(HMM_Vec3(0.0f, 0.0f,  0.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), 0.0f, 0.0f, 5.0f);
    camera.fp_cam = create_fp_camera(HMM_Vec3(0.0f, 0.0f,  3.0f), HMM_Vec3(0.0f, 1.0f,  0.0f), -90.f, 0.0f);
    camera.fp_enabled = false;
    camera.mouse_shown = true;
    camera.first_mouse = true;
    camera.show_help = false;
    
    return camera;
}

void update(struct camera* camera) {
    camera->frame_time = stm_laptime(&camera->time_stamp);
}

hmm_mat4 get_view_matrix(struct camera* camera) {
    if (camera->fp_enabled) {
        return get_view_matrix_fp(&camera->fp_cam);
    }
    else {
        return get_view_matrix_orbital(&camera->orbital_cam);
    }
}

void handle_input(struct camera* camera, const sapp_event* e) {
    if (e->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if (e->key_code == SAPP_KEYCODE_C) {
            camera->fp_enabled = !camera->fp_enabled;
        }
        else if (e->key_code == SAPP_KEYCODE_H) {
            camera->show_help = !camera->show_help;
        }
        else if (e->key_code == SAPP_KEYCODE_SPACE) {
            bool mouse_shown = sapp_mouse_shown();
            sapp_show_mouse(!mouse_shown);
        }
        else if (e->key_code == SAPP_KEYCODE_ESCAPE) {
            sapp_request_quit();
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
        handle_input_fp(&camera->fp_cam, e, mouse_offset, stm_ms(camera->frame_time));
    }
    else {
        handle_input_orbital(&camera->orbital_cam, e, mouse_offset);
    }
}

void render_help(struct camera* camera) {
    sdtx_canvas(sapp_width()*0.5f, sapp_height()*0.5f);
    sdtx_origin(0.25f, 0.25f);
    sdtx_home();

    if (!camera->show_help) {
        sdtx_color4b(0xff, 0xff, 0xff, 0xaf);
        sdtx_puts(  "Show help:\t'H'");
    }
    else {
        sdtx_color4b(0x00, 0xff, 0x00, 0xaf);
        sdtx_puts(  "Hide help:\t'H'\n\n");

        sdtx_printf("Frame Time:\t%.3f\n\n", stm_ms(camera->frame_time));

        if (camera->fp_enabled) {
            sdtx_puts(  "Orbital Cam\t[ ]\n"
                        "FP Cam\t\t[*]\n\n");
        }
        else {
            sdtx_puts(  "Orbital Cam\t[*]\n"
                        "FP Cam\t\t[ ]\n\n");
        }

        sdtx_puts("Switch Cam:\t'C'\n\n");

        if (camera->fp_enabled) {
            sdtx_puts(get_help_fp(&camera->fp_cam));
        }
        else {
            sdtx_puts(get_help_orbital(&camera->orbital_cam));
        }

        sdtx_puts("\nExit:\t\t'ESC'");
    }
    
    sdtx_draw();
}
