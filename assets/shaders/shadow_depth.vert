#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in vec2 aUV;
layout (location = 5) in vec4 aWeights;
layout (location = 6) in ivec4 aJoints;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

const int MAX_JOINTS = 32;
uniform bool useSkin;
uniform mat4 jointMatrices[MAX_JOINTS];

uniform bool useKelpSway;
uniform float kelpTime;
uniform float kelpSwayPhase;
uniform vec3 kelpSwayAxis;

void main() {
    vec3 pos = aPos;
    if (useKelpSway) {
        float height = clamp(aUV.y, 0.0, 1.0);
        float sway = sin(kelpTime * 1.3 + kelpSwayPhase) * 0.08 * height * height;
        pos += kelpSwayAxis * sway;
    }

    mat4 world = model;
    if (useSkin) {
        mat4 skin = aWeights.x * jointMatrices[aJoints.x] +
                    aWeights.y * jointMatrices[aJoints.y] +
                    aWeights.z * jointMatrices[aJoints.z] +
                    aWeights.w * jointMatrices[aJoints.w];
        world = model * skin;
    }
    gl_Position = lightSpaceMatrix * world * vec4(pos, 1.0);
}
