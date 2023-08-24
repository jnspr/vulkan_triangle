#version 450

layout (location = 0) out vec3 vColor;

struct Vertex {
    vec2 position;
    vec3 color;
};

Vertex triangle[3] = Vertex[] (
    Vertex(vec2( 0.0, -0.5), vec3(1.0, 0.0, 0.0)),
    Vertex(vec2( 0.5,  0.5), vec3(0.0, 1.0, 0.0)),
    Vertex(vec2(-0.5,  0.5), vec3(0.0, 0.0, 1.0))
);

void main() {
    gl_Position = vec4(triangle[gl_VertexIndex].position, 0.0, 1.0);
    vColor = triangle[gl_VertexIndex].color;
}
