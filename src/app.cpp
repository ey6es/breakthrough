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

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
int canvas_width, canvas_height;
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
    GLfloat matrix[] = {w, 0.0f, 0.0f, 0.0f, h, 0.0f, x, y, 1.0f};
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

EM_BOOL main_loop (double time, void* user_data) {
  static double last_time = time;
  double dt = time - last_time;
  last_time = time;

  emscripten_webgl_make_context_current(context);

  glClear(GL_COLOR_BUFFER_BIT);

  backdrop_program->draw_quad(0.5f, 0.5f, 0.5f, 0.5f);

  return true;
}

EM_BOOL on_canvas_resized (int event_type, const void* reserved, void* user_data) {
  emscripten_get_canvas_element_size("canvas", &canvas_width, &canvas_height);

  emscripten_webgl_make_context_current(context);

  glViewport(0, 0, canvas_width, canvas_height);

  return true;
}

EM_BOOL on_mouse_move (int event_type, const EmscriptenMouseEvent* mouse_event, void* user_data) {

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

  // we only need one vertex attribute buffer
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  const GLfloat bufferData[] {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(bufferData), bufferData, GL_STATIC_DRAW);

  quad_shader = load_shader(GL_VERTEX_SHADER, "rsrc/quad.vert");
  backdrop_program.reset(new shader_program("rsrc/backdrop.frag"));

  std::atexit(cleanup);

  EmscriptenFullscreenStrategy strategy {
    EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH,
    EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF,
    EMSCRIPTEN_FULLSCREEN_FILTERING_NEAREST,
    on_canvas_resized, nullptr};
  emscripten_enter_soft_fullscreen("canvas", &strategy);

  emscripten_set_mousemove_callback("canvas", nullptr, false, on_mouse_move);

  emscripten_request_animation_frame_loop(main_loop, nullptr);
}
