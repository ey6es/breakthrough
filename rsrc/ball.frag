precision mediump float;

varying vec2 tex_coord;

void main (void) {
  gl_FragColor = vec4(0.0, 0.0, 0.0, step(distance(tex_coord, vec2(0.5, 0.5)), 0.5));
}