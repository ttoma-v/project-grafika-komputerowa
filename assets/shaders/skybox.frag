#version 330 core
in vec3 LocalPos;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float depthTint;

void main() {
    vec3 dir = normalize(LocalPos);
    vec3 col = texture(skybox, dir).rgb;
    col *= vec3(0.35, 0.55, 0.75);
    col = mix(col, vec3(0.005, 0.02, 0.05), depthTint);
    FragColor = vec4(col, 1.0);
}
