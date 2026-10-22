#version 450 
out vec4 color;
in vec3 normal;
uniform vec3 sun_dir;
void main() {
    color = vec4(vec3(max(0.2,dot(normal, sun_dir))),1.0f);
}