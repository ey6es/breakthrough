precision mediump float;

uniform highp float aspect;

varying vec2 unit_coord;

void main (void) {
  vec2 coord = vec2(max(0.0, abs(unit_coord.x) - (aspect - 1.0)) * sign(unit_coord.x), unit_coord.y);
  vec3 normal = vec3(coord, sqrt(1.0 - dot(coord, coord)));

  const vec3 kDiffuseColor = vec3(0.75, 0.75, 0.75);
  const float kAmbient = 0.35;
  const vec3 kLightVector = vec3(0.57735, 0.57735, 0.57735);
  gl_FragColor = vec4(
    kDiffuseColor * (kAmbient + (1.0 - kAmbient) * max(dot(normal, kLightVector), 0.0)),
    step(distance(coord, vec2(0.0, 0.0)), 1.0));
}