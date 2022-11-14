uniform mat3 matrix;

attribute vec2 vertex;

varying vec2 tex_coord;

void main (void) {
  tex_coord = vertex + vec2(0.5, 0.5);
  vec3 pos = matrix * vec3(vertex, 1.0);
  gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}