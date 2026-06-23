#version 330 core
in float vHeightFactor;
out vec4 FragColor;

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(coord, coord);
    if (r2 > 1.0) discard;

    float soft = exp(-3.5 * r2);

    vec3 baseColor = vec3(0.75, 0.86, 0.88);

    float alpha = 0.22 * vHeightFactor * soft;

    FragColor = vec4(baseColor, alpha);
}

