precision highp float;

varying vec2 unit_coord;

void main (void) {
  vec3 normal = vec3(unit_coord, sqrt(1.0 - dot(unit_coord, unit_coord)));
  gl_FragColor = vec4(normal * 0.5 + vec3(0.5), step(distance(unit_coord, vec2(0.0, 0.0)), 1.0));
}