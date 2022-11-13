#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <emscripten.h>
#include <emscripten/html5.h>

#include "app.hpp"
#include "logic.hpp"

namespace {

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
int canvas_width, canvas_height, viewport_width, viewport_height;
float device_pixel_ratio;
GLuint buffer;
GLuint quad_shader;

EM_BOOL main_loop (double time, void* user_data) {
  static double last_time = time;
  constexpr double kSecondsPerMillisecond = 1.0 / 1000.0;
  double dt = (time - last_time) * kSecondsPerMillisecond;
  last_time = time;

  emscripten_webgl_make_context_current(context);

  glClear(GL_COLOR_BUFFER_BIT);

  tick(dt);

  return true;
}

EM_BOOL on_canvas_resized (int event_type, const void* reserved, void* user_data) {
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  emscripten_webgl_make_context_current(context);

  auto canvas_aspect = (float)canvas_width / canvas_height;
  if (canvas_aspect > kAspect) {
    viewport_width = (int)(canvas_height * kAspect);
    viewport_height = canvas_height;
    glViewport((canvas_width - viewport_width) / 2, 0, viewport_width, viewport_height);
  } else {
    viewport_width = canvas_width;
    viewport_height = (int)(canvas_width / kAspect);
    glViewport(0, (canvas_height - viewport_height) / 2, viewport_width, viewport_height);
  }

  return true;
}

EM_BOOL on_mouse_move (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {
  EmscriptenPointerlockChangeEvent pointerlock_event;
  emscripten_get_pointerlock_status(&pointerlock_event);

  if (pointerlock_event.isActive) {
    set_player_position(get_player_position() + mouse_event->movementX * device_pixel_ratio / viewport_width);
  } else {
    set_player_position((mouse_event->targetX * device_pixel_ratio - canvas_width / 2.0) / viewport_width);
  }

  return true;
}

EM_BOOL on_mouse_down (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {
  EmscriptenPointerlockChangeEvent pointerlock_event;
  emscripten_get_pointerlock_status(&pointerlock_event);

  if (!pointerlock_event.isActive) emscripten_request_pointerlock("canvas", false);

  maybe_release_player_ball();

  return true;
}

GLuint load_shader (GLenum shader_type, const char* filename) {
  GLuint shader = glCreateShader(shader_type);
  std::vector<std::string> lines;
  std::vector<const GLchar*> strings;
  std::vector<GLint> lengths;
  std::ifstream in(filename);
  while (in.good()) {
    lines.push_back(std::string());
    std::getline(in, lines.back());
    lengths.push_back(lines.back().length());
    strings.push_back(lines.back().data());
  }
  glShaderSource(shader, strings.size(), strings.data(), lengths.data());
  glCompileShader(shader);
  return shader;
}

void cleanup () {
  emscripten_webgl_make_context_current(context);

  glDeleteBuffers(1, &buffer);
  glDeleteShader(quad_shader);
}

}

int main () {
  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.alpha = false;
  attributes.depth = false;
  context = emscripten_webgl_create_context("canvas", &attributes);

  emscripten_webgl_make_context_current(context);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  const GLfloat kBufferData[] {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(kBufferData), kBufferData, GL_STATIC_DRAW);

  quad_shader = load_shader(GL_VERTEX_SHADER, "rsrc/quad.vert");
  backdrop_program.reset(new shader_program("rsrc/backdrop.frag"));
  paddle_program.reset(new shader_program("rsrc/paddle.frag"));
  ball_program.reset(new shader_program("rsrc/ball.frag"));

  std::atexit(cleanup);

  device_pixel_ratio = emscripten_get_device_pixel_ratio();

  EmscriptenFullscreenStrategy strategy {
    EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
    EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
    EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
    on_canvas_resized, nullptr};
  emscripten_enter_soft_fullscreen("canvas", &strategy);

  emscripten_set_mousemove_callback("canvas", nullptr, false, on_mouse_move);
  emscripten_set_mousedown_callback("canvas", nullptr, false, on_mouse_down);

  emscripten_request_animation_frame_loop(main_loop, nullptr);
}

std::unique_ptr<shader_program> backdrop_program;
std::unique_ptr<shader_program> paddle_program;
std::unique_ptr<shader_program> ball_program;

shader_program::shader_program (const char* fragment_filename) {
    program_ = glCreateProgram();
    glAttachShader(program_, quad_shader);
    glAttachShader(program_, fragment_shader_ = load_shader(GL_FRAGMENT_SHADER, fragment_filename));
    glLinkProgram(program_);
    vertex_location_ = glGetAttribLocation(program_, "vertex");
    matrix_location_ = glGetUniformLocation(program_, "matrix");
  }

shader_program::~shader_program () {
  emscripten_webgl_make_context_current(context);
  glDeleteProgram(program_);
  glDeleteShader(fragment_shader_);
}

void shader_program::draw_quad (GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
  glUseProgram(program_);
  constexpr GLfloat kXScale = 2.0f;
  constexpr GLfloat kYScale = kAspect * 2.0f;
  GLfloat matrix[] = {w * kXScale, 0.0f, 0.0f, 0.0f, h * kYScale, 0.0f, x * kXScale, y * kYScale, 1.0f};
  glUniformMatrix3fv(matrix_location_, 1, false, matrix);
  glEnableVertexAttribArray(vertex_location_);
  glVertexAttribPointer(vertex_location_, 2, GL_FLOAT, false, 0, nullptr);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}