precision mediump float;

uniform highp float time;

varying vec2 unit_coord;

void main (void) {
  const float kScrollSpeed = 0.005;
  vec2 scrolling_coord = unit_coord + vec2(time * kScrollSpeed);
  const float kQuarterRows = 10.0;
  vec2 checker_coord = fract(scrolling_coord * kQuarterRows) * 4.0 - vec2(2.0, 2.0);
  float direction = sign(checker_coord.x * checker_coord.y);
  const float kBlinkSpeed = 0.5;
  const float kBaseRadius = 0.25;
  float radius = kBaseRadius * (2.0 + sin(direction * time * kBlinkSpeed));
  vec2 dimple_coord = (abs(checker_coord) - vec2(1.0, 1.0)) * sign(checker_coord);
  float z2 = radius * radius - dot(dimple_coord, dimple_coord);
  const float kFlattening = 4.0;
  vec3 normal = (z2 < 0.0)
    ? vec3(0.0, 0.0, 1.0)
    : normalize(vec3(-dimple_coord, sqrt(z2) * kFlattening));

  const vec3 kDiffuseColor = vec3(0.2, 0.2, 0.2);
  const float kAmbient = 0.35;
  const vec3 kLightVector = vec3(0.57735, 0.57735, 0.57735);
  gl_FragColor = vec4(
      kDiffuseColor * (kAmbient + (1.0 - kAmbient) * max(dot(normal, kLightVector), 0.0)),
      1.0);
}