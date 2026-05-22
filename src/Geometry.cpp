#include "Geometry.h"

#include <cmath>

namespace {

void computeTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    std::vector<glm::vec3> tangentSum(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangentSum(vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const unsigned int i0 = indices[i];
        const unsigned int i1 = indices[i + 1];
        const unsigned int i2 = indices[i + 2];
        const Vertex& v0 = vertices[i0];
        const Vertex& v1 = vertices[i1];
        const Vertex& v2 = vertices[i2];

        const glm::vec3 e1 = v1.position - v0.position;
        const glm::vec3 e2 = v2.position - v0.position;
        const glm::vec2 duv1 = v1.uv - v0.uv;
        const glm::vec2 duv2 = v2.uv - v0.uv;
        const float det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (std::abs(det) < 1e-8f) continue;

        const float invDet = 1.0f / det;
        const glm::vec3 tangent = (e1 * duv2.y - e2 * duv1.y) * invDet;
        const glm::vec3 bitangent = (e2 * duv1.x - e1 * duv2.x) * invDet;

        tangentSum[i0] += tangent;
        tangentSum[i1] += tangent;
        tangentSum[i2] += tangent;
        bitangentSum[i0] += bitangent;
        bitangentSum[i1] += bitangent;
        bitangentSum[i2] += bitangent;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        Vertex& v = vertices[i];
        const glm::vec3 n = glm::normalize(v.normal);

        glm::vec3 t = tangentSum[i];
        if (glm::dot(t, t) < 1e-10f) {
            t = glm::abs(n.y) > 0.9f ? glm::cross(n, glm::vec3(1.0f, 0.0f, 0.0f))
                                     : glm::cross(n, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        t = glm::normalize(t - n * glm::dot(n, t));

        glm::vec3 b = bitangentSum[i];
        if (glm::dot(b, b) < 1e-10f) {
            b = glm::normalize(glm::cross(n, t));
        } else {
            b = glm::normalize(b - n * glm::dot(n, b));
        }

        v.tangent = t;
        v.bitangent = b;
    }
}

}

Mesh Geometry::makePlane(float width, float depth, int subdivisions) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    const int n = subdivisions + 1;
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const float fx = static_cast<float>(x) / subdivisions;
            const float fz = static_cast<float>(z) / subdivisions;
            Vertex v;
            v.position = glm::vec3((fx - 0.5f) * width, 0.0f, (fz - 0.5f) * depth);
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = glm::vec2(fx * 12.0f, fz * 12.0f);
            v.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            v.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
            verts.push_back(v);
        }
    }
    for (int z = 0; z < subdivisions; ++z) {
        for (int x = 0; x < subdivisions; ++x) {
            const unsigned int i0 = z * n + x;
            const unsigned int i1 = i0 + 1;
            const unsigned int i2 = i0 + n;
            const unsigned int i3 = i2 + 1;
            inds.push_back(i0);
            inds.push_back(i2);
            inds.push_back(i1);
            inds.push_back(i1);
            inds.push_back(i2);
            inds.push_back(i3);
        }
    }
    computeTangents(verts, inds);
    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}
