#ifndef APP_H
#define APP_H

#include <memory>
#include <GLES2/gl2.h>

constexpr float kAspect = 9.0f / 16.0f;

class shader_program {
public:
  shader_program (const char* fragment_filename);
  ~shader_program ();

  void draw_quad (GLfloat x, GLfloat y, GLfloat w, GLfloat h);

private:
  GLuint program_;
  GLuint fragment_shader_;
  GLint vertex_location_;
  GLint matrix_location_;
};

extern std::unique_ptr<shader_program> backdrop_program;
extern std::unique_ptr<shader_program> paddle_program;
extern std::unique_ptr<shader_program> ball_program;

#endif // APP_H