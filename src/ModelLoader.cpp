#include "ModelLoader.h"

#include "Geometry.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <unordered_map>

namespace {

const cgltf_accessor* findAccessor(const cgltf_primitive& prim, cgltf_attribute_type type) {
    for (size_t i = 0; i < prim.attributes_count; ++i) {
        if (prim.attributes[i].type == type) return prim.attributes[i].data;
    }
    return nullptr;
}

glm::mat4 cgltfMatrixToGlm(const cgltf_float m[16]) {
    glm::mat4 result(1.0f);
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result[col][row] = m[col * 4 + row];
        }
    }
    return result;
}

bool readVertex(const cgltf_primitive& prim, cgltf_size vertexIndex, const glm::mat3& normalMatrix, const glm::mat4& world,
                Vertex& out) {
    const cgltf_accessor* posAcc = findAccessor(prim, cgltf_attribute_type_position);
    if (!posAcc) return false;

    float pos[3] = {};
    if (cgltf_accessor_read_float(posAcc, vertexIndex, pos, 3) == 0) return false;
    out.position = glm::vec3(world * glm::vec4(pos[0], pos[1], pos[2], 1.0f));

    const cgltf_accessor* normAcc = findAccessor(prim, cgltf_attribute_type_normal);
    if (normAcc) {
        float nrm[3] = {};
        cgltf_accessor_read_float(normAcc, vertexIndex, nrm, 3);
        out.normal = glm::normalize(normalMatrix * glm::vec3(nrm[0], nrm[1], nrm[2]));
    } else {
        out.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const cgltf_accessor* uvAcc = findAccessor(prim, cgltf_attribute_type_texcoord);
    if (uvAcc) {
        float uv[2] = {};
        cgltf_accessor_read_float(uvAcc, vertexIndex, uv, 2);
        out.uv = glm::vec2(uv[0], uv[1]);
    } else {
        out.uv = glm::vec2(0.0f);
    }

    out.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    out.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);

    const cgltf_accessor* jointAcc = findAccessor(prim, cgltf_attribute_type_joints);
    if (jointAcc) {
        cgltf_uint j[4] = {0, 0, 0, 0};
        cgltf_accessor_read_uint(jointAcc, vertexIndex, j, 4);
        out.joints = glm::ivec4(static_cast<int>(j[0]), static_cast<int>(j[1]), static_cast<int>(j[2]),
                                static_cast<int>(j[3]));
    }

    const cgltf_accessor* weightAcc = findAccessor(prim, cgltf_attribute_type_weights);
    if (weightAcc) {
        float w[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        cgltf_accessor_read_float(weightAcc, vertexIndex, w, 4);
        const float sum = w[0] + w[1] + w[2] + w[3];
        if (sum > 1e-6f) {
            out.weights = glm::vec4(w[0], w[1], w[2], w[3]) / sum;
        } else {
            out.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    return true;
}

void appendPrimitive(const cgltf_primitive& prim, const glm::mat4& world, std::vector<Vertex>& vertices,
                     std::vector<unsigned int>& indices) {
    if (prim.type != cgltf_primitive_type_triangles) return;

    const cgltf_accessor* posAcc = findAccessor(prim, cgltf_attribute_type_position);
    if (!posAcc) return;

    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(world)));

    if (prim.indices) {
        for (cgltf_size t = 0; t < prim.indices->count; t += 3) {
            for (int corner = 0; corner < 3; ++corner) {
                const cgltf_size vi = cgltf_accessor_read_index(prim.indices, t + corner);
                Vertex v;
                if (!readVertex(prim, vi, normalMatrix, world, v)) continue;
                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(v);
            }
        }
        return;
    }

    for (cgltf_size i = 0; i < posAcc->count; i += 3) {
        for (int corner = 0; corner < 3; ++corner) {
            Vertex v;
            if (!readVertex(prim, i + corner, normalMatrix, world, v)) continue;
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back(v);
        }
    }
}

Texture2D makeFlatNormal() {
    Texture2D tex;
    tex.createRGBA(1, 1, {128, 128, 255, 255}, false);
    return tex;
}

Texture2D makeScalar(float value) {
    const unsigned char v = static_cast<unsigned char>(glm::clamp(value, 0.0f, 1.0f) * 255.0f);
    Texture2D tex;
    tex.createRGBA(1, 1, {v, v, v, 255}, false);
    return tex;
}

Texture2D loadCgltfImage(const cgltf_image* image, const std::string& modelDir) {
    if (!image) return {};

    if (image->buffer_view) {
        const uint8_t* data = cgltf_buffer_view_data(image->buffer_view);
        if (!data) return {};
        return Texture2D::loadFromMemory(data, static_cast<int>(image->buffer_view->size));
    }

    if (image->uri && image->uri[0] != '\0') {
        if (std::strncmp(image->uri, "data:", 5) == 0) {
            return {};
        }

        std::filesystem::path path = image->uri;
        if (!path.is_absolute()) path = std::filesystem::path(modelDir) / path;
        return Texture2D::loadFromFile(path.string());
    }

    return {};
}

Texture2D loadCgltfTexture(const cgltf_texture* texture, const std::string& modelDir,
                           std::unordered_map<const cgltf_image*, Texture2D>& cache) {
    if (!texture || !texture->image) return {};

    const cgltf_image* image = texture->image;
    const auto found = cache.find(image);
    if (found != cache.end()) return found->second;

    Texture2D loaded = loadCgltfImage(image, modelDir);
    if (loaded.id) cache.emplace(image, loaded);
    return loaded;
}

void readMaterial(const cgltf_primitive& prim, ModelLoader::SubMesh& sub, const std::string& modelDir,
                  std::unordered_map<const cgltf_image*, Texture2D>& imageCache) {
    if (!prim.material) return;

    const cgltf_material& mat = *prim.material;
    if (mat.name) sub.name = mat.name;
    sub.doubleSided = mat.double_sided;
    if (mat.has_pbr_metallic_roughness) {
        const cgltf_pbr_metallic_roughness& pbr = mat.pbr_metallic_roughness;
        sub.baseColorFactor = glm::vec3(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]);
        sub.metallicFactor = pbr.metallic_factor;
        sub.roughnessFactor = pbr.roughness_factor;
        if (pbr.base_color_texture.texture) {
            sub.albedo = loadCgltfTexture(pbr.base_color_texture.texture, modelDir, imageCache);
        }
    }
}

int nodeIndexOf(const cgltf_data* data, const cgltf_node* node) {
    if (!node) return -1;
    return static_cast<int>(node - data->nodes);
}

void parseSkeleton(const cgltf_data* data, ModelLoader::GltfModel& out) {
    const size_t nodeCount = data->nodes_count;
    out.nodeParent.assign(nodeCount, -1);
    out.nodeT.assign(nodeCount, glm::vec3(0.0f));
    out.nodeR.assign(nodeCount, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    out.nodeS.assign(nodeCount, glm::vec3(1.0f));

    for (size_t i = 0; i < nodeCount; ++i) {
        const cgltf_node& n = data->nodes[i];
        if (n.has_translation) out.nodeT[i] = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
        if (n.has_rotation) out.nodeR[i] = glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]);
        if (n.has_scale) out.nodeS[i] = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
        for (size_t c = 0; c < n.children_count; ++c) {
            out.nodeParent[nodeIndexOf(data, n.children[c])] = static_cast<int>(i);
        }
    }

    if (data->skins_count == 0) return;

    const cgltf_skin& skin = data->skins[0];
    out.hasSkin = true;
    out.jointCount = static_cast<int>(skin.joints_count);
    out.jointNodes.resize(skin.joints_count);
    out.inverseBind.resize(skin.joints_count, glm::mat4(1.0f));
    for (size_t j = 0; j < skin.joints_count; ++j) {
        out.jointNodes[j] = nodeIndexOf(data, skin.joints[j]);
        if (skin.inverse_bind_matrices) {
            float m[16] = {};
            cgltf_accessor_read_float(skin.inverse_bind_matrices, j, m, 16);
            out.inverseBind[j] = cgltfMatrixToGlm(m);
        }
    }
}

void parseAnimations(const cgltf_data* data, ModelLoader::GltfModel& out) {
    for (size_t a = 0; a < data->animations_count; ++a) {
        const cgltf_animation& anim = data->animations[a];
        ModelLoader::Animation result;
        if (anim.name) result.name = anim.name;

        for (size_t c = 0; c < anim.channels_count; ++c) {
            const cgltf_animation_channel& ch = anim.channels[c];
            if (!ch.sampler || !ch.target_node) continue;

            ModelLoader::AnimationSampler sampler;
            sampler.targetNode = nodeIndexOf(data, ch.target_node);
            switch (ch.target_path) {
                case cgltf_animation_path_type_translation: sampler.path = 0; break;
                case cgltf_animation_path_type_rotation: sampler.path = 1; break;
                case cgltf_animation_path_type_scale: sampler.path = 2; break;
                default: continue;
            }

            const cgltf_accessor* input = ch.sampler->input;
            const cgltf_accessor* output = ch.sampler->output;
            const size_t keys = input->count;
            sampler.times.resize(keys);
            sampler.values.resize(keys, glm::vec4(0.0f));
            const int comps = (sampler.path == 1) ? 4 : 3;
            for (size_t k = 0; k < keys; ++k) {
                float t = 0.0f;
                cgltf_accessor_read_float(input, k, &t, 1);
                sampler.times[k] = t;
                result.duration = std::max(result.duration, t);
                float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                cgltf_accessor_read_float(output, k, v, comps);
                sampler.values[k] = glm::vec4(v[0], v[1], v[2], v[3]);
            }
            result.channels.push_back(std::move(sampler));
        }
        out.animations.push_back(std::move(result));
    }
}

glm::mat4 nodeLocalMatrix(const glm::vec3& t, const glm::quat& r, const glm::vec3& s) {
    return glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
}

glm::mat4 bindGlobal(const ModelLoader::GltfModel& m, int node) {
    glm::mat4 result(1.0f);
    while (node >= 0) {
        result = nodeLocalMatrix(m.nodeT[node], m.nodeR[node], m.nodeS[node]) * result;
        node = m.nodeParent[node];
    }
    return result;
}

std::string toLowerAscii(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

bool isChestGoldAsset(const std::string& path) {
    const std::string lower = toLowerAscii(std::filesystem::path(path).filename().string());
    return lower.find("chest_gold") != std::string::npos;
}

bool isFxGlowSubmeshName(const std::string& name) {
    if (name.empty()) return false;
    const std::string lower = toLowerAscii(name);
    static constexpr const char* kKeywords[] = {"glow", "fx", "beam", "ray", "plane", "flare", "light_flare"};
    for (const char* keyword : kKeywords) {
        if (lower.find(keyword) != std::string::npos) return true;
    }
    return false;
}

std::string resolveSubmeshName(const cgltf_node& node, const cgltf_mesh& mesh, const std::string& materialName) {
    if (mesh.name && mesh.name[0]) return mesh.name;
    if (node.name && node.name[0]) return node.name;
    return materialName;
}

}

namespace ModelLoader {

glm::mat4 swimTransform(const glm::vec3& position, const glm::vec3& pathTangent) {
    glm::vec3 forward = glm::normalize(pathTangent);
    if (!std::isfinite(forward.x)) forward = glm::vec3(0.0f, 0.0f, -1.0f);

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    if (glm::length(right) < 1e-4f) right = glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::vec3 up = glm::normalize(glm::cross(right, forward));

    glm::mat4 m(1.0f);
    m[0] = glm::vec4(-right, 0.0f);
    m[1] = glm::vec4(up, 0.0f);
    m[2] = glm::vec4(forward, 0.0f);
    m[3] = glm::vec4(position, 1.0f);
    return m;
}

bool loadGlb(const std::string& path, GltfModel& out, float targetSize) {
    cgltf_options options{};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success) {
        std::cerr << "GLB parse failed: " << path << '\n';
        return false;
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        std::cerr << "GLB buffer load failed: " << path << '\n';
        cgltf_free(data);
        return false;
    }

    parseSkeleton(data, out);
    parseAnimations(data, out);

    const std::string modelDir = std::filesystem::path(path).parent_path().string();
    std::unordered_map<const cgltf_image*, Texture2D> imageCache;

    struct RawSub {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        SubMesh material;
    };

    std::vector<RawSub> raws;

    for (size_t n = 0; n < data->nodes_count; ++n) {
        const cgltf_node& node = data->nodes[n];
        if (!node.mesh) continue;

        glm::mat4 world(1.0f);
        if (!node.skin) {
            cgltf_float worldMatrix[16];
            cgltf_node_transform_world(&node, worldMatrix);
            world = cgltfMatrixToGlm(worldMatrix);
        }

        const cgltf_mesh& mesh = *node.mesh;
        for (size_t p = 0; p < mesh.primitives_count; ++p) {
            RawSub raw;
            appendPrimitive(mesh.primitives[p], world, raw.vertices, raw.indices);
            if (raw.vertices.empty() || raw.indices.empty()) continue;
            readMaterial(mesh.primitives[p], raw.material, modelDir, imageCache);
            raw.material.name = resolveSubmeshName(node, mesh, raw.material.name);
            raws.push_back(std::move(raw));
        }
    }

    cgltf_free(data);

    if (raws.empty()) {
        std::cerr << "GLB contains no triangle geometry: " << path << '\n';
        return false;
    }

    std::vector<glm::mat4> bindJoint(out.jointCount, glm::mat4(1.0f));
    for (int j = 0; j < out.jointCount; ++j) {
        bindJoint[j] = bindGlobal(out, out.jointNodes[j]) * out.inverseBind[j];
    }
    auto bindPos = [&](const Vertex& v) -> glm::vec3 {
        if (!out.hasSkin) return v.position;
        glm::mat4 skin = v.weights.x * bindJoint[v.joints.x] + v.weights.y * bindJoint[v.joints.y] +
                         v.weights.z * bindJoint[v.joints.z] + v.weights.w * bindJoint[v.joints.w];
        return glm::vec3(skin * glm::vec4(v.position, 1.0f));
    };

    glm::vec3 minB(std::numeric_limits<float>::max());
    glm::vec3 maxB(std::numeric_limits<float>::lowest());
    for (const auto& raw : raws) {
        for (const auto& v : raw.vertices) {
            const glm::vec3 p = bindPos(v);
            minB = glm::min(minB, p);
            maxB = glm::max(maxB, p);
        }
    }
    const glm::vec3 center = (minB + maxB) * 0.5f;
    const glm::vec3 extent = maxB - minB;
    const float maxAxis = std::max(extent.x, std::max(extent.y, extent.z));
    const float scale = maxAxis > 1e-6f ? targetSize / maxAxis : 1.0f;
    const glm::mat4 normalize =
        glm::scale(glm::mat4(1.0f), glm::vec3(scale)) * glm::translate(glm::mat4(1.0f), -center);
    out.normalizeMatrix = normalize;

    out.submeshes.clear();
    out.submeshes.reserve(raws.size());
    size_t totalTris = 0;
    const bool filterFxGlow = isChestGoldAsset(path);
    size_t subMeshIndex = 0;
    for (auto& raw : raws) {
        const std::string& subName = raw.material.name.empty() ? std::string("(unnamed)") : raw.material.name;
        std::cout << "[Model Loader] Loaded sub-mesh [" << subMeshIndex << "]: \"" << subName
                  << "\" (vertices: " << raw.vertices.size() << ")\n";

        if (filterFxGlow && isFxGlowSubmeshName(subName)) {
            std::cout << "[Model Loader] Skipping FX sub-mesh [" << subMeshIndex << "]: \"" << subName << "\"\n";
            ++subMeshIndex;
            continue;
        }
        ++subMeshIndex;

        glm::vec3 centroid(0.0f);
        for (const auto& v : raw.vertices) centroid += glm::vec3(normalize * glm::vec4(bindPos(v), 1.0f));
        centroid /= static_cast<float>(raw.vertices.size());

        if (!out.hasSkin) {
            for (auto& v : raw.vertices) v.position = glm::vec3(normalize * glm::vec4(v.position, 1.0f));
        }

        Geometry::computeTangents(raw.vertices, raw.indices);
        Geometry::addDoubleSidedFaces(raw.vertices, raw.indices);

        SubMesh sub;
        sub.name = raw.material.name;
        sub.baseColorFactor = raw.material.baseColorFactor;
        sub.metallicFactor = raw.material.metallicFactor;
        sub.roughnessFactor = raw.material.roughnessFactor;
        sub.doubleSided = raw.material.doubleSided;
        sub.albedo = raw.material.albedo;
        sub.bindCentroid = centroid;
        sub.mesh.upload(raw.vertices, raw.indices);
        totalTris += raw.indices.size() / 3;
        out.submeshes.push_back(std::move(sub));
    }

    out.whiteAlbedo = makeScalar(1.0f);
    out.flatNormal = makeFlatNormal();
    out.whiteScalar = makeScalar(1.0f);
    out.valid = true;

    std::cout << "Loaded GLB: " << path << " (" << out.submeshes.size() << " parts, " << totalTris << " tris, "
              << out.jointCount << " joints, " << out.animations.size() << " anims)\n";
    return true;
}

const Animation* findAnimation(const GltfModel& model, const std::string& nameSubstr) {
    for (const auto& anim : model.animations) {
        if (anim.name.find(nameSubstr) != std::string::npos) return &anim;
    }
    return nullptr;
}

void computeJointMatrices(const GltfModel& model, const Animation* anim, float timeSeconds,
                          std::vector<glm::mat4>& outMatrices) {
    outMatrices.assign(model.jointCount, glm::mat4(1.0f));
    if (!model.hasSkin) return;

    const size_t nodeCount = model.nodeParent.size();
    std::vector<glm::vec3> localT = model.nodeT;
    std::vector<glm::quat> localR = model.nodeR;
    std::vector<glm::vec3> localS = model.nodeS;

    if (anim && anim->duration > 0.0f) {
        const float t = std::fmod(timeSeconds, anim->duration);
        for (const auto& ch : anim->channels) {
            if (ch.targetNode < 0 || ch.times.empty()) continue;

            size_t k1 = 0;
            while (k1 + 1 < ch.times.size() && ch.times[k1 + 1] < t) ++k1;
            const size_t k2 = std::min(k1 + 1, ch.times.size() - 1);
            const float t1 = ch.times[k1];
            const float t2 = ch.times[k2];
            const float a = (t2 > t1) ? glm::clamp((t - t1) / (t2 - t1), 0.0f, 1.0f) : 0.0f;

            if (ch.path == 1) {
                const glm::vec4& q1 = ch.values[k1];
                const glm::vec4& q2 = ch.values[k2];
                const glm::quat r1(q1.w, q1.x, q1.y, q1.z);
                const glm::quat r2(q2.w, q2.x, q2.y, q2.z);
                localR[ch.targetNode] = glm::normalize(glm::slerp(r1, r2, a));
            } else {
                const glm::vec4 v = glm::mix(ch.values[k1], ch.values[k2], a);
                if (ch.path == 0) localT[ch.targetNode] = glm::vec3(v);
                else localS[ch.targetNode] = glm::vec3(v);
            }
        }
    }

    std::vector<glm::mat4> global(nodeCount, glm::mat4(1.0f));
    std::vector<char> done(nodeCount, 0);
    std::function<glm::mat4(int)> resolve = [&](int node) -> glm::mat4 {
        if (done[node]) return global[node];
        const glm::mat4 local = nodeLocalMatrix(localT[node], localR[node], localS[node]);
        const int parent = model.nodeParent[node];
        global[node] = (parent >= 0) ? resolve(parent) * local : local;
        done[node] = 1;
        return global[node];
    };

    for (int j = 0; j < model.jointCount; ++j) {
        const int node = model.jointNodes[j];
        outMatrices[j] = model.normalizeMatrix * resolve(node) * model.inverseBind[j];
    }
}

}
