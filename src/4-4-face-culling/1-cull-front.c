//------------------------------------------------------------------------------
//  Face Culling (1)
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "hmm/HandmadeMath.h"
#include "1-cull-front.glsl.h"
#define LOPGL_APP_IMPL
#include "../lopgl_app.h"

/* application state */
static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    lopgl_setup();

    float cube_vertices[] = {
        // triangles defined CCW
        // back face
        0.5f, -0.5f, -0.5f,  4.0f, 0.0f, // bottom-right    
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
        0.5f,  0.5f, -0.5f,  4.0f, 4.0f, // top-right              
        -0.5f,  0.5f, -0.5f,  0.0f, 4.0f, // top-left
        0.5f,  0.5f, -0.5f,  4.0f, 4.0f, // top-right
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left                
        // front face
        0.5f,  0.5f,  0.5f,  4.0f, 4.0f, // top-right
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
        0.5f, -0.5f,  0.5f,  4.0f, 0.0f, // bottom-right        
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
        0.5f,  0.5f,  0.5f,  4.0f, 4.0f, // top-right
        -0.5f,  0.5f,  0.5f,  0.0f, 4.0f, // top-left        
        // left face
        -0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // bottom-left
        -0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // top-right
        -0.5f,  0.5f, -0.5f,  4.0f, 4.0f, // top-left       
        -0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // top-right
        -0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // bottom-left
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
        // right face
        0.5f,  0.5f, -0.5f,  4.0f, 4.0f, // top-right      
        0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // top-left
        0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // bottom-right          
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
        0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // bottom-right
        0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // top-left
        // bottom face          
        0.5f, -0.5f,  0.5f,  4.0f, 0.0f, // bottom-left
        -0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // top-right
        0.5f, -0.5f, -0.5f,  4.0f, 4.0f, // top-left        
        -0.5f, -0.5f, -0.5f,  0.0f, 4.0f, // top-right
        0.5f, -0.5f,  0.5f,  4.0f, 0.0f, // bottom-left
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
        // top face
        0.5f,  0.5f, -0.5f,  4.0f, 4.0f, // top-right
        -0.5f,  0.5f, -0.5f,  0.0f, 4.0f, // top-left
        0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // bottom-right                 
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // bottom-left  
        0.5f,  0.5f,  0.5f,  4.0f, 0.0f, // bottom-right
        -0.5f,  0.5f, -0.5f,  0.0f, 4.0f  // top-left              
    };

    sg_buffer cube_buffer = sg_make_buffer(&(sg_buffer_desc){
        .size = sizeof(cube_vertices),
        .content = cube_vertices,
        .label = "cube-vertices"
    });
    
    state.bind.vertex_buffers[0] = cube_buffer;

    /* checkerboard texture */
    uint32_t pixels[4*4] = {
        0xFFBBBBBB, 0xFF444444, 0xFFBBBBBB, 0xFF444444,
        0xFF444444, 0xFFBBBBBB, 0xFF444444, 0xFFBBBBBB,
        0xFFBBBBBB, 0xFF444444, 0xFFBBBBBB, 0xFF444444,
        0xFF444444, 0xFFBBBBBB, 0xFF444444, 0xFFBBBBBB,
    };

    state.bind.fs_images[SLOT_diffuse_texture] = sg_make_image(&(sg_image_desc){
        .width = 4,
        .height = 4,
        .content.subimage[0][0] = {
            .ptr = pixels,
            .size = sizeof(pixels)
        },
        .label = "cube-texture"
    });

    /* create shader from code-generated sg_shader_desc */
    sg_shader simple_shd = sg_make_shader(simple_shader_desc());

    /* create a pipeline object for the cubes */
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = simple_shd,
        /* if the vertex layout doesn't have gaps, don't need to provide strides and offsets */
        .layout = {
            .attrs = {
                [ATTR_vs_a_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_a_tex_coords].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .depth_stencil = {
            .depth_compare_func = SG_COMPAREFUNC_LESS,
            .depth_write_enabled = true,
        },
        .rasterizer = {
            .cull_mode = SG_CULLMODE_FRONT,
            .face_winding = SG_FACEWINDING_CCW
        },
        .label = "pipeline"
    });
    

    /* a pass action to clear framebuffer */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action=SG_ACTION_CLEAR, .val={0.1f, 0.1f, 0.1f, 1.0f} }
    };
}

void frame(void) {
    lopgl_update();

    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());

    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    hmm_mat4 view = lopgl_view_matrix();
    hmm_mat4 projection = HMM_Perspective(lopgl_fov(), (float)sapp_width() / (float)sapp_height(), 0.1f, 100.0f);

    vs_params_t vs_params = {
        .model = HMM_Mat4d(1.0f),
        .view = view,
        .projection = projection
    };

    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
    sg_draw(0, 36, 1);

    lopgl_render_help();

    sg_end_pass();
    sg_commit();
}

void event(const sapp_event* e) {
    lopgl_handle_input(e);
}

void cleanup(void) {
    lopgl_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .gl_force_gles2 = true,
        .window_title = "Cull Front (LearnOpenGL)",
    };
}
