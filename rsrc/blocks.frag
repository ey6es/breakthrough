precision mediump float;

uniform sampler2D texture;

varying vec2 tex_coord;

void main (void) {
  gl_FragColor = texture2D(texture, tex_coord);
}