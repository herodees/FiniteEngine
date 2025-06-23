#type vertex
// Vertex shader
attribute vec3 aPos;
uniform vec2 test;
void main() {
    gl_Position = vec4(aPos, 1.0);
}

#type fragment
// Fragment shader
precision mediump float;

void main() {
    gl_FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}