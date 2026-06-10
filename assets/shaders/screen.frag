#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneColor;
uniform sampler2D flowMap;
uniform float time;
uniform float distortionStrength;

void main() {
    vec2 flow = texture(flowMap, TexCoords * 3.0 + vec2(time * 0.02, time * 0.015)).rg;
    flow = flow * 2.0 - 1.0;
    float phase = fract(time * 0.12);
    vec2 offset = flow * distortionStrength * (0.5 + 0.5 * sin(phase * 6.28318));

    vec2 uv1 = TexCoords + offset;
    vec2 uv2 = TexCoords + offset * 0.65 + vec2(0.003 * sin(time * 1.3), 0.002 * cos(time * 0.9));

    vec2 uv1c = clamp(uv1, 0.0, 1.0);
    vec2 uv2c = clamp(uv2, 0.0, 1.0);
    vec3 c1 = texture(sceneColor, uv1c).rgb;
    vec3 c2 = texture(sceneColor, uv2c).rgb;
    vec3 color = mix(c1, c2, 0.35);

    float vignette = mix(0.82, 1.0, 1.0 - smoothstep(0.55, 0.95, length(TexCoords - 0.5) * 1.2));
    color *= vignette;

    FragColor = vec4(color, 1.0);
}
