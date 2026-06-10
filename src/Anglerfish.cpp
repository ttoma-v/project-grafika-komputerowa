#include "Anglerfish.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace {
constexpr float kMetallic = 0.0f;
constexpr float kRoughness = 0.9f;
const glm::vec3 kLureEmissive{0.10f, 0.42f, 0.52f};
constexpr const char* kSwimAnimation = "Swimming_Normal";
constexpr const char* kLureSubmesh = "Light";
}

bool Anglerfish::load(const std::string& glbPath, float targetSize) {
    if (!ModelLoader::loadGlb(glbPath, model_, targetSize)) return false;

    for (size_t i = 0; i < model_.submeshes.size(); ++i) {
        auto& sub = model_.submeshes[i];
        sub.metallicFactor = kMetallic;
        sub.roughnessFactor = kRoughness;
        if (sub.name == kLureSubmesh) {
            sub.emissive = kLureEmissive;
            lureIndex_ = static_cast<int>(i);
        }
    }
    anim_ = ModelLoader::findAnimation(model_, kSwimAnimation);
    return true;
}

void Anglerfish::setupCircularPath(const glm::vec3& center, float radius, int points) {
    path_.closed = true;
    path_.controlPoints.clear();
    for (int i = 0; i < points; ++i) {
        const float ang = (glm::two_pi<float>() * static_cast<float>(i)) / static_cast<float>(points);
        path_.controlPoints.push_back(center + glm::vec3(std::cos(ang) * radius, 0.0f, std::sin(ang) * radius));
    }
    path_.rebuild(glm::vec3(0.0f, 1.0f, 0.0f));
}

void Anglerfish::update(float time) {
    const CurveFrame frame = path_.frameAt(time * pathSpeed);
    transform_ = ModelLoader::swimTransform(frame.position, frame.tangent);

    if (!model_.valid) return;

    ModelLoader::computeJointMatrices(model_, anim_, time, joints_);
    if (lureIndex_ >= 0) {
        lurePosition_ = glm::vec3(transform_ * glm::vec4(model_.submeshes[lureIndex_].bindCentroid, 1.0f));
    }
}
