#include "Geometry.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

void addBackFacesForStrip(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, unsigned int base,
                          unsigned int vertCount, unsigned int indStart) {
    const unsigned int backBase = static_cast<unsigned int>(verts.size());
    for (unsigned int v = 0; v < vertCount; ++v) {
        Vertex flipped = verts[base + v];
        flipped.normal = -flipped.normal;
        flipped.tangent = -flipped.tangent;
        flipped.bitangent = -flipped.bitangent;
        verts.push_back(flipped);
    }

    const unsigned int indEnd = static_cast<unsigned int>(inds.size());
    for (unsigned int i = indStart; i < indEnd; i += 3) {
        inds.push_back(backBase + (inds[i] - base));
        inds.push_back(backBase + (inds[i + 2] - base));
        inds.push_back(backBase + (inds[i + 1] - base));
    }
}

void anchorVerticesToGround(std::vector<Vertex>& verts) {
    if (verts.empty()) return;
    float minY = verts[0].position.y;
    for (const auto& v : verts) minY = std::min(minY, v.position.y);
    for (auto& v : verts) v.position.y -= minY;
}

}

void Geometry::computeTangents(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
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
            t = glm::abs(n.y) > 0.9f ? glm::cross(n, glm::vec3(1.0f, 0.0f, 0.0f)) : glm::cross(n, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        t = glm::normalize(t - n * glm::dot(n, t));

        glm::vec3 b = bitangentSum[i];
        if (glm::dot(b, b) < 1e-10f) {
            b = glm::normalize(glm::cross(n, t));
        } else {
            b = glm::normalize(b - n * glm::dot(n, b) - t * glm::dot(t, b));
        }

        v.tangent = t;
        v.bitangent = b;
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

namespace {

float seabedHeight(float x, float z) {
    const float base = std::sin(x * 0.2f) * 0.4f + std::cos(z * 0.15f) * 0.32f;
    const float duneWaves = std::sin(x * 0.55f + z * 0.21f) * 0.9f + std::cos(x * 0.33f - z * 0.47f) * 0.7f;
    const float ridge = std::sin(z * 0.6f) * std::cos(x * 0.18f) * 0.45f;
    const float cross = std::sin((x + z) * 0.28f) * 0.35f;

    const float pitA = std::exp(-(((x + 12.0f) * (x + 12.0f)) / 90.0f + ((z - 16.0f) * (z - 16.0f)) / 70.0f));
    const float pitB = std::exp(-(((x - 9.0f) * (x - 9.0f)) / 65.0f + ((z - 23.0f) * (z - 23.0f)) / 80.0f));
    const float pitC = std::exp(-(((x - 14.0f) * (x - 14.0f)) / 85.0f + ((z + 14.0f) * (z + 14.0f)) / 75.0f));
    const float pitD = std::exp(-(((x + 16.0f) * (x + 16.0f)) / 95.0f + ((z + 22.0f) * (z + 22.0f)) / 90.0f));
    const float pits = -0.85f * pitA - 0.65f * pitB - 0.75f * pitC - 0.6f * pitD;

    return base + duneWaves * 0.38f + ridge * 0.3f + cross + pits;
}

glm::vec3 seabedNormal(float x, float z) {
    const float eps = 0.25f;
    const float dydx = (seabedHeight(x + eps, z) - seabedHeight(x - eps, z)) / (2.0f * eps);
    const float dydz = (seabedHeight(x, z + eps) - seabedHeight(x, z - eps)) / (2.0f * eps);
    return glm::normalize(glm::vec3(-dydx, 1.0f, -dydz));
}

}

Mesh Geometry::makeSeabed(float width, float depth, int subdivisions) {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    const int n = subdivisions + 1;
    constexpr float kReferenceSeabedSize = 80.0f;
    constexpr float kReferenceUvTiling = 12.0f;
    const float uvTilingX = kReferenceUvTiling * (width / kReferenceSeabedSize);
    const float uvTilingZ = kReferenceUvTiling * (depth / kReferenceSeabedSize);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const float fx = static_cast<float>(x) / subdivisions;
            const float fz = static_cast<float>(z) / subdivisions;
            const float px = (fx - 0.5f) * width;
            const float pz = (fz - 0.5f) * depth;
            const float py = seabedHeight(px, pz);
            const glm::vec3 normal = seabedNormal(px, pz);

            Vertex v;
            v.position = glm::vec3(px, py, pz);
            v.normal = normal;
            v.uv = glm::vec2(fx * uvTilingX, fz * uvTilingZ);
            const glm::vec3 tangent = glm::normalize(glm::vec3(1.0f, std::cos(px * 0.2f) * 0.08f, 0.0f));
            v.tangent = tangent;
            v.bitangent = glm::normalize(glm::cross(normal, tangent));
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

namespace {

struct CoralBuilder {
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;

    void addBranch(const glm::vec3& base, const glm::vec3& dir, float radius, int depth) {
        if (depth <= 0) return;
        const glm::vec3 top = base + dir * (0.6f + dist(rng));
        const glm::vec3 side = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f) + glm::vec3(dist(rng))));
        const glm::vec3 side2 = glm::normalize(glm::cross(dir, side));
        const unsigned int baseIdx = static_cast<unsigned int>(verts.size());
        const glm::vec3 n = glm::normalize(glm::cross(side, side2));
        auto pushV = [&](const glm::vec3& p, const glm::vec2& uv) {
            verts.push_back({p, n, side, side2, uv});
        };
        pushV(base + side * radius, {0, 0});
        pushV(base - side * radius, {1, 0});
        pushV(base + side2 * radius, {0, 1});
        pushV(top, {0.5f, 0.5f});
        const unsigned int i = baseIdx;
        inds.insert(inds.end(), {i, i + 1, i + 3, i + 1, i + 2, i + 3, i, i + 3, i + 2});
        addBranch(top, glm::normalize(dir + glm::vec3(dist(rng), dist(rng) * 0.5f + 0.4f, dist(rng))), radius * 0.7f, depth - 1);
        if (depth > 2)
            addBranch(top, glm::normalize(dir + glm::vec3(dist(rng), dist(rng), dist(rng))), radius * 0.65f, depth - 1);
    }
};

}

Mesh Geometry::makeLowPolyCoral(float scale, unsigned int seed) {
    CoralBuilder builder{std::mt19937(seed), std::uniform_real_distribution<float>(-0.3f, 0.3f)};
    builder.addBranch(glm::vec3(0.0f),
                      glm::vec3(builder.dist(builder.rng) * 0.2f, 1.0f, builder.dist(builder.rng) * 0.2f), 0.15f * scale, 4);
    auto& verts = builder.verts;
    auto& inds = builder.inds;
    for (auto& v : verts) v.position *= scale;
    anchorVerticesToGround(verts);
    computeTangents(verts, inds);
    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Geometry::makeKelpSegment() {
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    const int segments = 5;

    const auto addStrip = [&](const glm::vec3& widthAxis) {
        const unsigned int base = static_cast<unsigned int>(verts.size());
        const unsigned int indStart = static_cast<unsigned int>(inds.size());
        const glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), widthAxis));
        for (int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / segments;
            const float w = 0.08f * (1.0f - t * 0.4f);
            const glm::vec3 center(0.0f, t * 1.2f, 0.0f);
            verts.push_back({center - widthAxis * w, normal, glm::normalize(widthAxis), {0, 1, 0}, {0, t}});
            verts.push_back({center + widthAxis * w, normal, glm::normalize(widthAxis), {0, 1, 0}, {1, t}});
        }
        const unsigned int vertCount = static_cast<unsigned int>(verts.size()) - base;
        for (int i = 0; i < segments; ++i) {
            const unsigned int a = base + static_cast<unsigned int>(i * 2);
            inds.push_back(a);
            inds.push_back(a + 2);
            inds.push_back(a + 1);
            inds.push_back(a + 1);
            inds.push_back(a + 2);
            inds.push_back(a + 3);
        }
        addBackFacesForStrip(verts, inds, base, vertCount, indStart);
    };

    addStrip(glm::vec3(1.0f, 0.0f, 0.0f));
    addStrip(glm::vec3(0.0f, 0.0f, 1.0f));

    anchorVerticesToGround(verts);
    computeTangents(verts, inds);
    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Geometry::makeKelpAlongSpline(const ParallelTransportSpline& spline, float maxWidth) {
    const std::vector<CurveFrame>& frames = spline.frames;
    if (frames.size() < 2) return {};

    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    const int ringCount = static_cast<int>(frames.size());
    const int stride = ringCount > 64 ? 2 : 1;

    const auto addBladeStrip = [&](const glm::vec3& widthAxisAt, int ringIndex) {
        const CurveFrame& frame = frames[static_cast<size_t>(ringIndex)];
        const glm::vec3 widthAxis = glm::normalize(widthAxisAt);
        const glm::vec3 faceNormal = glm::normalize(glm::cross(frame.tangent, widthAxis));
        const glm::vec3 bitangent = glm::normalize(glm::cross(faceNormal, widthAxis));
        const float t = static_cast<float>(ringIndex) / static_cast<float>(ringCount - 1);
        const float w = maxWidth * (1.0f - t * 0.45f);

        verts.push_back({frame.position - widthAxis * w, faceNormal, widthAxis, bitangent, {0.0f, t}});
        verts.push_back({frame.position + widthAxis * w, faceNormal, widthAxis, bitangent, {1.0f, t}});
    };

    for (int strip = 0; strip < 1; ++strip) {
        const unsigned int base = static_cast<unsigned int>(verts.size());
        const unsigned int indStart = static_cast<unsigned int>(inds.size());

        std::vector<int> ringIndices;
        for (int ring = 0; ring < ringCount; ring += stride) ringIndices.push_back(ring);
        if (ringIndices.back() != ringCount - 1) ringIndices.push_back(ringCount - 1);

        for (int ring : ringIndices) {
            const CurveFrame& frame = frames[static_cast<size_t>(ring)];
            const glm::vec3 widthAxis = strip == 0 ? frame.binormal : frame.normal;
            addBladeStrip(widthAxis, ring);
        }

        const unsigned int vertCount = static_cast<unsigned int>(verts.size()) - base;
        for (size_t i = 0; i + 1 < ringIndices.size(); ++i) {
            const unsigned int a = base + static_cast<unsigned int>(i * 2);
            inds.push_back(a);
            inds.push_back(a + 2);
            inds.push_back(a + 1);
            inds.push_back(a + 1);
            inds.push_back(a + 2);
            inds.push_back(a + 3);
        }
        addBackFacesForStrip(verts, inds, base, vertCount, indStart);
    }

    computeTangents(verts, inds);
    Mesh mesh;
    mesh.upload(verts, inds);
    return mesh;
}

Mesh Geometry::makeRock(float scale, unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    const int slices = 6;
    const int stacks = 4;
    for (int y = 0; y <= stacks; ++y) {
        for (int x = 0; x <= slices; ++x) {
            const float phi = static_cast<float>(y) / stacks * 3.14159265f;
            const float theta = static_cast<float>(x) / slices * 6.28318530f;
            glm::vec3 p(std::sin(phi) * std::cos(theta), std::cos(phi) * 0.5f, std::sin(phi) * std::sin(theta));
            p += glm::vec3(dist(rng), dist(rng), dist(rng)) * 0.15f;
            p.y = std::abs(p.y);
            p *= scale;
            verts.push_back({p, glm::normalize(p), {1, 0, 0}, {0, 1, 0}, {static_cast<float>(x), static_cast<float>(y)}});
        }
    }
    anchorVerticesToGround(verts);
    for (int y = 0; y < stacks; ++y) {
        for (int x = 0; x < slices; ++x) {
            const unsigned int i0 = y * (slices + 1) + x;
            const unsigned int i1 = i0 + 1;
            const unsigned int i2 = i0 + slices + 1;
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

void Geometry::addDoubleSidedFaces(std::vector<Vertex>& verts, std::vector<unsigned int>& inds) {
    if (verts.empty() || inds.empty()) return;

    const unsigned int frontCount = static_cast<unsigned int>(verts.size());
    const unsigned int indStart = static_cast<unsigned int>(inds.size());

    verts.reserve(verts.size() * 2);
    inds.reserve(inds.size() * 2);

    for (unsigned int v = 0; v < frontCount; ++v) {
        Vertex flipped = verts[v];
        flipped.normal = -flipped.normal;
        flipped.tangent = -flipped.tangent;
        flipped.bitangent = -flipped.bitangent;
        verts.push_back(flipped);
    }

    for (unsigned int i = 0; i < indStart; i += 3) {
        inds.push_back(frontCount + inds[i]);
        inds.push_back(frontCount + inds[i + 2]);
        inds.push_back(frontCount + inds[i + 1]);
    }
}
