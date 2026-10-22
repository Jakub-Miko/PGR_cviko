#version 450 

layout(location = 0) in vec4 pos;
layout(location = 1) in vec3 norm;
uniform mat4 mvp;
uniform mat4 normal_mat;

out vec3 normal;
void main() {
    gl_Position = mvp * pos;
    normal = normalize(normal_mat * vec4(norm, 0.0f)).xyz;
}

