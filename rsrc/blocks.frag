precision mediump float;

uniform highp float aspect;

uniform sampler2D texture;
uniform float field_cols;
uniform float field_rows;

varying vec2 tex_coord;
varying vec2 unit_coord;

void main (void) {
  float block_aspect = field_rows * aspect / field_cols;
  vec2 block_coord =
    fract(tex_coord * vec2(field_cols, field_rows)) * vec2(2.0 * block_aspect, 2.0) -
    vec2(block_aspect, 1.0);
  const float kBevelRadius = 0.5;
  vec2 abs_coord = max(
    vec2(0.0, 0.0),
    abs(block_coord) - vec2(block_aspect - kBevelRadius, 1.0 - kBevelRadius));
  vec2 signed_coord =
    sign(block_coord) *
    mix(vec2(abs_coord.x, 0.0), vec2(0.0, abs_coord.y), step(abs_coord.x, abs_coord.y)) /
    kBevelRadius;
  vec3 normal = vec3(signed_coord, sqrt(1.0 - dot(signed_coord, signed_coord)));
  gl_FragColor = vec4(normal * 0.5 + vec3(0.5), texture2D(texture, tex_coord).a);
}