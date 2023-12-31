#version 450

layout (location = 0) in vec2 iPosition;
layout (location = 1) in vec3 iColor;
layout (location = 0) out vec3 vColor;

void main() {
    gl_Position = vec4(iPosition, 0.0, 1.0);
    vColor = iColor;
}
