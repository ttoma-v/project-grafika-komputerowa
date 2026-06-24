#pragma once

#include "Mesh.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace ModelLoader {

struct SubMesh {
    Mesh mesh;
    std::string name;
    glm::vec3 baseColorFactor{1.0f};
    glm::vec3 emissive{0.0f};
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.85f;
    bool doubleSided = false;
    glm::vec3 bindCentroid{0.0f};
    Texture2D albedo;
};

struct AnimationSampler {
    std::vector<float> times;
    std::vector<glm::vec4> values;
    int targetNode = -1;
    int path = 0;
};

struct Animation {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationSampler> channels;
};

struct GltfModel {
    std::vector<SubMesh> submeshes;
    Texture2D whiteAlbedo;
    Texture2D flatNormal;
    Texture2D whiteScalar;
    bool valid = false;

    bool hasSkin = false;
    int jointCount = 0;
    std::vector<int> jointNodes;
    std::vector<glm::mat4> inverseBind;
    glm::mat4 normalizeMatrix{1.0f};

    std::vector<int> nodeParent;
    std::vector<glm::vec3> nodeT;
    std::vector<glm::quat> nodeR;
    std::vector<glm::vec3> nodeS;

    std::vector<Animation> animations;
};

bool loadGlb(const std::string& path, GltfModel& out, float targetSize = 2.0f);

glm::mat4 swimTransform(const glm::vec3& position, const glm::vec3& pathTangent);

const Animation* findAnimation(const GltfModel& model, const std::string& nameSubstr);

void computeJointMatrices(const GltfModel& model, const Animation* anim, float timeSeconds,
                          std::vector<glm::mat4>& outMatrices);

}
