#include "PTF.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>

static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

static glm::vec3 controlPointAt(const std::vector<glm::vec3>& points, int index, bool closed) {
    const int n = static_cast<int>(points.size());
    if (n == 0) return glm::vec3(0.0f);
    if (closed) {
        const int wrapped = ((index % n) + n) % n;
        return points[static_cast<size_t>(wrapped)];
    }
    return points[static_cast<size_t>(std::clamp(index, 0, n - 1))];
}

glm::mat4 CurveFrame::matrix() const {
    return glm::mat4(glm::vec4(binormal, 0.0f), glm::vec4(normal, 0.0f), glm::vec4(-tangent, 0.0f),
                     glm::vec4(position, 1.0f));
}

glm::vec3 ParallelTransportSpline::evaluate(float t) const {
    const int n = static_cast<int>(controlPoints.size());
    if (n == 0) return glm::vec3(0.0f);

    if (closed) {
        if (n < 3) return controlPoints[0];
        const int segments = n;
        const float wrappedT = t - std::floor(t);
        const float ft = wrappedT * static_cast<float>(segments);
        const int seg = std::min(static_cast<int>(ft), segments - 1);
        const float localT = ft - static_cast<float>(seg);
        return catmullRom(controlPointAt(controlPoints, seg - 1, true), controlPointAt(controlPoints, seg, true),
                          controlPointAt(controlPoints, seg + 1, true), controlPointAt(controlPoints, seg + 2, true),
                          localT);
    }

    if (n < 4) return controlPoints[0];
    const int segments = n - 3;
    const float ft = glm::clamp(t, 0.0f, 0.9999f) * static_cast<float>(segments);
    const int seg = std::min(static_cast<int>(ft), segments - 1);
    const float localT = ft - static_cast<float>(seg);
    return catmullRom(controlPoints[static_cast<size_t>(seg)], controlPoints[static_cast<size_t>(seg + 1)],
                      controlPoints[static_cast<size_t>(seg + 2)], controlPoints[static_cast<size_t>(seg + 3)], localT);
}

void ParallelTransportSpline::rebuild(const glm::vec3& initialNormal) {
    frames.clear();
    const int n = static_cast<int>(controlPoints.size());
    if (closed ? n < 3 : n < 4) return;

    const int segments = closed ? n : n - 3;
    const int totalSamples = closed ? segments * samplesPerSegment : segments * samplesPerSegment + 1;

    glm::vec3 prevTangent{};
    glm::vec3 normal = glm::normalize(initialNormal);

    for (int s = 0; s < totalSamples; ++s) {
        const float t = closed ? static_cast<float>(s) / static_cast<float>(totalSamples)
                               : static_cast<float>(s) / static_cast<float>(totalSamples - 1);
        const glm::vec3 pos = evaluate(t);

        const float eps = 0.002f;
        const glm::vec3 posNext = evaluate(t + eps);
        glm::vec3 tangent = glm::normalize(posNext - pos);
        if (s == 0) {
            if (std::abs(glm::dot(tangent, normal)) > 0.95f) normal = glm::normalize(glm::cross(tangent, glm::vec3(1.0f, 0.0f, 0.0f)));
            prevTangent = tangent;
        } else {
            const glm::vec3 axis = glm::cross(prevTangent, tangent);
            const float axisLen = glm::length(axis);
            if (axisLen > 1e-5f) {
                const float angle = std::acos(glm::clamp(glm::dot(prevTangent, tangent), -1.0f, 1.0f));
                const glm::quat rot = glm::angleAxis(angle, axis / axisLen);
                normal = glm::normalize(rot * normal);
            }
            prevTangent = tangent;
        }

        const glm::vec3 binormal = glm::normalize(glm::cross(tangent, normal));
        normal = glm::normalize(glm::cross(binormal, tangent));
        frames.push_back({pos, tangent, normal, binormal});
    }
}

CurveFrame ParallelTransportSpline::frameAt(float t) const {
    if (frames.empty()) return {};

    const int frameCount = static_cast<int>(frames.size());
    float ft = 0.0f;
    int i = 0;
    int j = 0;
    if (closed) {
        ft = (t - std::floor(t)) * static_cast<float>(frameCount);
        i = static_cast<int>(ft) % frameCount;
        j = (i + 1) % frameCount;
    } else {
        ft = glm::clamp(t, 0.0f, 1.0f) * static_cast<float>(frameCount - 1);
        i = static_cast<int>(ft);
        j = std::min(i + 1, frameCount - 1);
    }
    const float a = ft - static_cast<float>(i);
    CurveFrame out;
    out.position = glm::mix(frames[i].position, frames[j].position, a);
    out.tangent = glm::normalize(glm::mix(frames[i].tangent, frames[j].tangent, a));
    out.normal = glm::normalize(glm::mix(frames[i].normal, frames[j].normal, a));
    out.binormal = glm::normalize(glm::cross(out.tangent, out.normal));
    out.normal = glm::normalize(glm::cross(out.binormal, out.tangent));
    return out;
}
