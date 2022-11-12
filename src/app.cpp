#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

namespace {

constexpr double kAspect = 9.0 / 16.0;

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
int canvas_width, canvas_height, viewport_width, viewport_height;
double device_pixel_ratio;
GLuint buffer;
GLuint quad_shader;

GLuint load_shader (GLenum shader_type, const char* filename);

class shader_program {
public:

  shader_program (const char* fragment_filename) {
    program_ = glCreateProgram();
    glAttachShader(program_, quad_shader);
    glAttachShader(program_, fragment_shader_ = load_shader(GL_FRAGMENT_SHADER, fragment_filename));
    glLinkProgram(program_);
    vertex_location_ = glGetAttribLocation(program_, "vertex");
    matrix_location_ = glGetUniformLocation(program_, "matrix");
  }

  ~shader_program () {
    emscripten_webgl_make_context_current(context);
    glDeleteProgram(program_);
    glDeleteShader(fragment_shader_);
  }

  void draw_quad (GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    glUseProgram(program_);
    constexpr GLfloat kXScale = 2.0f;
    constexpr GLfloat kYScale = kAspect * 2.0f;
    GLfloat matrix[] = {w * kXScale, 0.0f, 0.0f, 0.0f, h * kYScale, 0.0f, x * kXScale, y * kYScale, 1.0f};
    glUniformMatrix3fv(matrix_location_, 1, false, matrix);
    glEnableVertexAttribArray(vertex_location_);
    glVertexAttribPointer(vertex_location_, 2, GL_FLOAT, false, 0, nullptr);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

private:

  GLuint program_;
  GLuint fragment_shader_;
  GLint vertex_location_;
  GLint matrix_location_;
};

std::unique_ptr<shader_program> backdrop_program;
std::unique_ptr<shader_program> paddle_program;

constexpr float kPaddleWidth = 0.2f;
constexpr float kPaddleHeight = 0.05f;

double player_position = 0.0;

EM_BOOL main_loop (double time, void* user_data) {
  static double last_time = time;
  double dt = time - last_time;
  last_time = time;

  emscripten_webgl_make_context_current(context);

  glClear(GL_COLOR_BUFFER_BIT);

  backdrop_program->draw_quad(0.0f, 0.0f, 1.0f, 1.0f / kAspect);

  paddle_program->draw_quad(player_position, -0.5f / kAspect + kPaddleHeight * 0.5f, kPaddleWidth, kPaddleHeight);

  return true;
}

EM_BOOL on_canvas_resized (int event_type, const void* reserved, void* user_data) {
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  emscripten_webgl_make_context_current(context);

  auto canvas_aspect = (double)canvas_width / canvas_height;
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
    player_position += mouse_event->movementX * device_pixel_ratio / viewport_width;
  } else {
    player_position = (mouse_event->targetX * device_pixel_ratio - canvas_width / 2.0) / viewport_width;
  }

  constexpr double kMaxPos = (1.0 + kPaddleWidth) * 0.5;
  player_position = std::min(std::max(player_position, -kMaxPos), kMaxPos);

  return true;
}

EM_BOOL on_mouse_down (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {
  EmscriptenPointerlockChangeEvent pointerlock_event;
  emscripten_get_pointerlock_status(&pointerlock_event);

  if (!pointerlock_event.isActive) emscripten_request_pointerlock("canvas", false);
  
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
