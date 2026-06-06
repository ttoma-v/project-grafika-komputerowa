#pragma once

#include <glm/glm.hpp>
#include <vector>

struct CurveFrame {
    glm::vec3 position;
    glm::vec3 tangent;
    glm::vec3 normal;
    glm::vec3 binormal;
    glm::mat4 matrix() const;
};

class ParallelTransportSpline {
public:
    std::vector<glm::vec3> controlPoints;
    std::vector<CurveFrame> frames;
    int samplesPerSegment = 24;
    bool closed = false;

    void rebuild(const glm::vec3& initialNormal = glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 evaluate(float t) const;
    CurveFrame frameAt(float t) const;
};
