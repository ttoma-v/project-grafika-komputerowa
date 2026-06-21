#version 330 core
in vec3 LocalPos;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float depthTint;

void main() {
    vec3 dir = normalize(LocalPos);
    vec3 col = texture(skybox, dir).rgb;
    col *= vec3(0.35, 0.55, 0.75);
    vec3 fogColor = vec3(0.06, 0.21, 0.23);
    col = mix(col, fogColor, depthTint);
    float lowerBlend = smoothstep(0.1, -0.5, dir.y);
    col = mix(col, fogColor, lowerBlend);
    FragColor = vec4(col, 1.0);
}
