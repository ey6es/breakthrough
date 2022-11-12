uniform mat3 matrix;

attribute vec2 vertex;

void main (void) {
  vec3 pos = matrix * vec3(vertex, 1.0);
  gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}