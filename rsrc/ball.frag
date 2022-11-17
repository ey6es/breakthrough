precision mediump float;

varying vec2 unit_coord;

void main (void) {
  vec3 normal = vec3(unit_coord, sqrt(1.0 - dot(unit_coord, unit_coord)));
  const vec3 kDiffuseColor = vec3(1.0, 1.0, 1.0);

  const float kAmbient = 0.35;
  const vec3 kLightVector = vec3(0.57735, 0.57735, 0.57735);
  gl_FragColor = vec4(
    kDiffuseColor * (kAmbient + (1.0 - kAmbient) * max(dot(normal, kLightVector), 0.0)),
    step(distance(unit_coord, vec2(0.0, 0.0)), 1.0));
}