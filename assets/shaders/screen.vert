#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in vec2 aUV;

out vec2 TexCoords;

void main() {
    TexCoords = aUV;
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
}
