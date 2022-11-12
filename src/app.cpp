#include <iostream>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

namespace {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  int width, height;
}

EM_BOOL main_loop (double time, void* user_data) {
  static double last_time = time;
  double dt = time - last_time;
  last_time = time;

  emscripten_webgl_make_context_current(context);

  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return true;
}

EM_BOOL on_canvas_resized (int event_type, const void* reserved, void* user_data) {
  emscripten_get_canvas_element_size("canvas", &width, &height);

  emscripten_webgl_make_context_current(context);

  glViewport(0, 0, width, height);

  return true;
}

EM_BOOL on_mouse_move (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {

  return true;
}

int main () {
  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.alpha = false;
  attributes.depth = false;
  context = emscripten_webgl_create_context("canvas", &attributes);

  EmscriptenFullscreenStrategy strategy {
    EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
    EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
    EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
    on_canvas_resized, nullptr};
  emscripten_enter_soft_fullscreen("canvas", &strategy);

  emscripten_set_mousemove_callback("canvas", nullptr, false, on_mouse_move);

  emscripten_request_animation_frame_loop(main_loop, nullptr);
}
