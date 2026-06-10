#pragma once

#include "ModelLoader.h"
#include "PTF.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// Anglerfish entity: owns its glTF model, swim path, skeletal animation
// state and the glowing lure that drives a dynamic light.
class Anglerfish {
public:
    bool load(const std::string& glbPath, float targetSize = 2.0f);

    // Builds a closed circular swim path on the XZ plane.
    void setupCircularPath(const glm::vec3& center, float radius, int points);

    // Advances animation and recomputes transform/joints/lure for the frame.
    void update(float time);

    bool valid() const { return model_.valid; }
    const glm::mat4& transform() const { return transform_; }
    const std::vector<glm::mat4>& joints() const { return joints_; }
    const std::vector<ModelLoader::SubMesh>& submeshes() const { return model_.submeshes; }
    const ModelLoader::GltfModel& model() const { return model_; }

    bool hasLure() const { return lureIndex_ >= 0; }
    const glm::vec3& lureLightPosition() const { return lurePosition_; }

    float pathSpeed = 0.045f;

private:
    ModelLoader::GltfModel model_;
    const ModelLoader::Animation* anim_ = nullptr;
    ParallelTransportSpline path_;

    int lureIndex_ = -1;
    std::vector<glm::mat4> joints_;
    glm::mat4 transform_{1.0f};
    glm::vec3 lurePosition_{0.0f};
};
