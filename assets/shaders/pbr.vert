#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec3 aBitangent;
layout (location = 4) in vec2 aUV;
layout (location = 5) in vec4 aWeights;
layout (location = 6) in ivec4 aJoints;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_JOINTS = 32;
uniform bool useSkin;
uniform mat4 jointMatrices[MAX_JOINTS];

void main() {
    mat4 world = model;
    if (useSkin) {
        mat4 skin = aWeights.x * jointMatrices[aJoints.x] +
                    aWeights.y * jointMatrices[aJoints.y] +
                    aWeights.z * jointMatrices[aJoints.z] +
                    aWeights.w * jointMatrices[aJoints.w];
        world = model * skin;
    }

    FragPos = vec3(world * vec4(aPos, 1.0));
    TexCoords = aUV;
    mat3 normalMatrix = mat3(transpose(inverse(world)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
