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
bool context_lost = false;
int canvas_width, canvas_height;
double canvas_offset = 0.0;
float device_pixel_ratio;
GLuint buffer;
GLuint quad_shader;
GLuint block_texture;

EM_BOOL main_loop (double time, void* user_data) {
  static double last_time = time;
  constexpr double kSecondsPerMillisecond = 1.0 / 1000.0;
  double dt = (time - last_time) * kSecondsPerMillisecond;
  last_time = time;

  if (context_lost) return false;

  emscripten_webgl_make_context_current(context);

  tick(dt);

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

void init_context () {
  emscripten_webgl_make_context_current(context);

  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  const GLfloat kBufferData[] {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(kBufferData), kBufferData, GL_STATIC_DRAW);

  quad_shader = load_shader(GL_VERTEX_SHADER, "rsrc/quad.vert");
  backdrop_program.reset(new shader_program("rsrc/backdrop.frag"));
  paddle_program.reset(new shader_program("rsrc/paddle.frag"));
  ball_program.reset(new shader_program("rsrc/ball.frag"));

  blocks_program.reset(new shader_program("rsrc/blocks.frag"));
  blocks_program->set_uniform("texture", 0);
  blocks_program->set_uniform("field_rows", (float)kFieldRows);
  blocks_program->set_uniform("field_cols", (float)kFieldCols);

  glGenTextures(1, &block_texture);
  glBindTexture(GL_TEXTURE_2D, block_texture);
  reset_blocks();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  emscripten_request_animation_frame_loop(main_loop, nullptr);
}

EM_BOOL on_canvas_resized (int event_type, const void* reserved, void* user_data) {
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  double w, h;
  emscripten_get_element_css_size("canvas", &w, &h);
  canvas_offset = (w * device_pixel_ratio  - canvas_width) * 0.5;

  emscripten_webgl_make_context_current(context);

  glViewport(0, 0, canvas_width, canvas_height);

  return true;
}

void update_player_position (int target_x) {
  set_player_position((target_x * device_pixel_ratio - canvas_offset) / canvas_width - 0.5f);
}

EM_BOOL on_mouse_move (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {
  EmscriptenPointerlockChangeEvent pointerlock_event;
  emscripten_get_pointerlock_status(&pointerlock_event);

  if (pointerlock_event.isActive) {
    set_player_position(get_player_position() + mouse_event->movementX * device_pixel_ratio / canvas_width);
  } else {
    update_player_position(mouse_event->targetX);
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

EM_BOOL on_touch_start (int event_type, const EmscriptenTouchEvent* touch_event, void* user_data) {
  update_player_position(touch_event->touches[0].targetX);
  maybe_release_player_ball();
  return true;
}

EM_BOOL on_touch_move (int event_type, const EmscriptenTouchEvent* touch_event, void* user_data) {
  update_player_position(touch_event->touches[0].targetX);
  return true;
}

EM_BOOL on_webglcontext_lost (int event_type, const void* reserved, void* user_data) {
  context_lost = true;
  return true;
}

EM_BOOL on_webglcontext_restored (int event_type, const void* reserved, void* user_data) {
  init_context();
  context_lost = false;
  return true;
}

void cleanup () {
  if (context_lost) return;

  emscripten_webgl_make_context_current(context);

  glDeleteBuffers(1, &buffer);
  glDeleteShader(quad_shader);
  glDeleteTextures(1, &block_texture);
} 

}

int main () {
  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.alpha = false;
  attributes.depth = false;
  context = emscripten_webgl_create_context("canvas", &attributes);

  init_context();

  std::atexit(cleanup);

  device_pixel_ratio = emscripten_get_device_pixel_ratio();

  emscripten_set_canvas_element_size("canvas", canvas_width = kAspect * 160, canvas_height = 160);

  EmscriptenFullscreenStrategy strategy {
    EMSCRIPTEN_FULLSCREEN_SCALE_ASPECT,
    EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
    EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
    on_canvas_resized, nullptr};
  emscripten_enter_soft_fullscreen("canvas", &strategy);

  emscripten_set_mousemove_callback("canvas", nullptr, false, on_mouse_move);
  emscripten_set_mousedown_callback("canvas", nullptr, false, on_mouse_down);

  emscripten_set_touchstart_callback("canvas", nullptr, true, on_touch_start);
  emscripten_set_touchmove_callback("canvas", nullptr, true, on_touch_move);

  emscripten_set_webglcontextlost_callback("canvas", nullptr, false, on_webglcontext_lost);
  emscripten_set_webglcontextrestored_callback("canvas", nullptr, false, on_webglcontext_restored);
}

void reset_blocks () {
  unsigned char texture_data[kFieldRows * kFieldCols * 4];
  unsigned char* it = texture_data;
  auto write_rgb = [&](float r, float g, float b) {
    *it++ = (unsigned char)(255.0f * r);
    *it++ = (unsigned char)(255.0f * g);
    *it++ = (unsigned char)(255.0f * b);
  };
  for (auto row = 0; row < kFieldRows; ++row) {
    for (auto col = 0; col < kFieldCols; ++col) {
      auto hue = 5.0f * (row + col + 1.0f) / (kFieldCols + kFieldRows);
      auto range = (int)hue;
      auto level = hue - range;
      switch (range) {
        case 0: write_rgb(1.0f, level, 0.0f); break;
        case 1: write_rgb(1.0f - level, 1.0f, 0.0f); break;
        case 2: write_rgb(0.0f, 1.0f, level); break;
        case 3: write_rgb(0.0f, 1.0f - level, 1.0f); break;
        case 4: write_rgb(level, 0.0f, 1.0f); break;
      }
      *it++ = get_block_state(row, col) ? 0x0 : 0xFF;
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kFieldCols, kFieldRows, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
}

void clear_block (int row, int col) {
  unsigned char texture_data[] {0, 0, 0, 0};
  glTexSubImage2D(GL_TEXTURE_2D, 0, col, row, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
}

shader_program::shader_program (const char* fragment_filename) {
    program_ = glCreateProgram();
    glAttachShader(program_, quad_shader);
    glAttachShader(program_, fragment_shader_ = load_shader(GL_FRAGMENT_SHADER, fragment_filename));
    glLinkProgram(program_);
    vertex_location_ = glGetAttribLocation(program_, "vertex");
    matrix_location_ = glGetUniformLocation(program_, "matrix");
    aspect_location_ = glGetUniformLocation(program_, "aspect");
  }

shader_program::~shader_program () {
  if (context_lost) return;

  emscripten_webgl_make_context_current(context);
  glDeleteProgram(program_);
  glDeleteShader(fragment_shader_);
}

void shader_program::set_uniform (const char* name, int value) const {
  auto location = glGetUniformLocation(program_, name);
  glUseProgram(program_);
  glUniform1i(location, value);
}

void shader_program::set_uniform (const char* name, float value) const {
  auto location = glGetUniformLocation(program_, name);
  glUseProgram(program_);
  glUniform1f(location, value);
}

void shader_program::draw_quad (GLfloat x, GLfloat y, GLfloat w, GLfloat h) const {
  glUseProgram(program_);
  constexpr GLfloat kXScale = 2.0f;
  constexpr GLfloat kYScale = kAspect * 2.0f;
  GLfloat matrix[] = {w * kXScale, 0.0f, 0.0f, 0.0f, h * kYScale, 0.0f, x * kXScale, y * kYScale, 1.0f};
  glUniformMatrix3fv(matrix_location_, 1, false, matrix);
  glUniform1f(aspect_location_, w / h);
  glEnableVertexAttribArray(vertex_location_);
  glVertexAttribPointer(vertex_location_, 2, GL_FLOAT, false, 0, nullptr);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

std::unique_ptr<shader_program> backdrop_program;
std::unique_ptr<shader_program> paddle_program;
std::unique_ptr<shader_program> ball_program;
std::unique_ptr<shader_program> blocks_program;