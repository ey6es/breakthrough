uniform mat3 matrix;
uniform float aspect;

attribute vec2 vertex;

varying vec2 tex_coord;
varying vec2 unit_coord;

void main (void) {
  tex_coord = vertex + vec2(0.5, 0.5);
  unit_coord = vec2(vertex.x * 2.0 * aspect, vertex.y * 2.0);
  vec3 pos = matrix * vec3(vertex, 1.0);
  gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}