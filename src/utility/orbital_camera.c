#include "orbital_camera.h"

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
