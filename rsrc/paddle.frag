precision mediump float;

uniform highp float aspect;

varying vec2 unit_coord;

void main (void) {
  vec2 coord = vec2(max(0.0, abs(unit_coord.x) - (aspect - 1.0)) * sign(unit_coord.x), unit_coord.y);
  vec3 normal = vec3(coord, sqrt(1.0 - dot(coord, coord)));
  gl_FragColor = vec4(normal * 0.5 + vec3(0.5), step(distance(coord, vec2(0.0, 0.0)), 1.0));
}