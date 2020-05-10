#pragma once
/*
    The typical debug UI overlay useful for most sokol-app samples
*/
#include "sokol_app.h"

typedef void (draw_callback)(void);

extern void ui_setup(int sample_count);
extern void ui_shutdown(void);
extern void ui_draw(draw_callback* draw_options);
extern void ui_event(const sapp_event* e);
