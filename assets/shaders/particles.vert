#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

out float vHeightFactor;

void main() {
    vec3 worldPos = aPos;

    float jitter = sin(dot(worldPos.xz, vec2(0.27, 0.43)) + time * 0.8) * 0.12;
    worldPos.xz += vec2(jitter * 0.25, jitter * 0.15);

    gl_Position = projection * view * vec4(worldPos, 1.0);

    float dist = length(worldPos);
    float size = mix(5.0, 2.0, clamp(dist / 45.0, 0.0, 1.0));
    gl_PointSize = size;

    float h = clamp(1.0 - worldPos.y / 10.0, 0.0, 1.0);
    vHeightFactor = h;
}

