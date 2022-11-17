#ifndef APP_H
#define APP_H

#include <memory>
#include <GLES2/gl2.h>

constexpr float kAspect = 9.0f / 16.0f;

void reset_blocks ();
void clear_block (int row, int col);

class shader_program {
public:
  shader_program (const char* fragment_filename);
  ~shader_program ();

  void set_uniform (const char* name, int value) const;
  void set_uniform (const char* name, float value) const;

  void draw_quad (GLfloat x, GLfloat y, GLfloat w, GLfloat h) const;

private:
  GLuint program_;
  GLuint fragment_shader_;
  GLint vertex_location_;
  GLint matrix_location_;
  GLint aspect_location_;
  GLint time_location_;
};

extern std::unique_ptr<shader_program> backdrop_program;
extern std::unique_ptr<shader_program> paddle_program;
extern std::unique_ptr<shader_program> ball_program;
extern std::unique_ptr<shader_program> blocks_program;

#endif // APP_H