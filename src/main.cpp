#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Geometry.h"
#include "GlLoader.h"
#include "Mesh.h"
#include "PTF.h"
#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int gWidth = 1280;
static int gHeight = 720;
static bool gFirstMouse = true;
static double gLastX = 0.0;
static double gLastY = 0.0;
static bool gKeys[1024]{};

static std::string assetPath(const std::string& rel) {
    const fs::path fromExe = fs::current_path() / "assets" / rel;
    if (fs::exists(fromExe)) return fromExe.string();
#ifdef PROJECT_ROOT_DIR
    const fs::path fromRoot = fs::path(PROJECT_ROOT_DIR) / "assets" / rel;
    if (fs::exists(fromRoot)) return fromRoot.string();
#endif
    return (fs::path("assets") / rel).string();
}

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    gWidth = w;
    gHeight = h;
    glViewport(0, 0, w, h);
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) gKeys[key] = true;
        if (action == GLFW_RELEASE) gKeys[key] = false;
    }
}

static void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (gFirstMouse) {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }
    const float dx = static_cast<float>(xpos - gLastX);
    const float dy = static_cast<float>(gLastY - ypos);
    gLastX = xpos;
    gLastY = ypos;
    extern Camera gCamera;
    gCamera.processMouse(dx, dy);
}

Camera gCamera;

static Mesh makeSkyboxCube() {
    std::vector<Vertex> verts = {
        {{-1, 1, -1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{-1, -1, -1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{1, -1, -1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{1, 1, -1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{1, 1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{1, -1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{-1, -1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{-1, 1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
    };
    std::vector<unsigned int> inds = {0, 1, 2, 0, 2, 3, 3, 2, 4, 2, 5, 4, 4, 5, 6, 4, 6, 7, 7, 6, 1, 7, 1, 0, 0, 3, 7, 3, 4, 7, 1, 6, 5, 1, 0, 6};
    Mesh m;
    m.upload(verts, inds);
    return m;
}

struct DrawObject {
    Mesh mesh;
    glm::mat4 transform{1.0f};
    glm::vec3 albedoTint{1.0f};
    float metallic = 0.05f;
    float roughness = 0.85f;
};

struct ShadowMap {
    unsigned int fbo = 0;
    unsigned int depthMap = 0;
    const int size = 4096;

    void create() {
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy() {
        if (depthMap) glDeleteTextures(1, &depthMap);
        if (fbo) glDeleteFramebuffers(1, &fbo);
    }
};

static void drawPBR(const Shader& shader, const Mesh& mesh, const glm::mat4& model, const glm::vec3& tint, float metallic,
                    float roughness, const Texture2D& albedo, const Texture2D& normal, const Texture2D& metal,
                    const Texture2D& rough, const glm::vec3& emissive = glm::vec3(0.0f)) {
    shader.setMat4("model", model);
    shader.setVec3("materialAlbedoTint", tint);
    shader.setFloat("materialMetallic", metallic);
    shader.setFloat("materialRoughness", roughness);
    shader.setVec3("materialEmissive", emissive);
    albedo.bind(0);
    normal.bind(1);
    metal.bind(2);
    rough.bind(3);
    mesh.draw();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Bathyscaphe - Deep Underwater", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "OpenGL loader failed\n";
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    gCamera.position = glm::vec3(0.0f, 2.2f, 0.0f);
    gCamera.yaw = 0.0f;
    gCamera.pitch = glm::radians(-4.0f);

    Shader skyShader;
    Shader shadowShader;
    Shader pbrShader;
    const std::string skyVert = assetPath("shaders/skybox.vert");
    const std::string skyFrag = assetPath("shaders/skybox.frag");
    const std::string shadowVert = assetPath("shaders/shadow_depth.vert");
    const std::string shadowFrag = assetPath("shaders/shadow_depth.frag");
    const std::string pbrVert = assetPath("shaders/pbr.vert");
    const std::string pbrFrag = assetPath("shaders/pbr.frag");

    if (!shadowShader.loadFromFiles(shadowVert, shadowFrag) || !skyShader.loadFromFiles(skyVert, skyFrag) ||
        !pbrShader.loadFromFiles(pbrVert, pbrFrag)) {
        std::cerr << "Shader load failed. Run exe from build/Release or ensure assets/ is nearby.\n";
        std::cerr << "Tried: " << pbrVert << '\n';
        return 1;
    }
    std::cout << "Shaders OK. Assets from: " << fs::current_path() << '\n';

    auto sandAlbedo = ProceduralTextures::makeSandAlbedo();
    auto sandNormal = ProceduralTextures::makeSandNormal();
    auto sandMetal = ProceduralTextures::makeMetallic(64, 0.02f);
    auto sandRough = ProceduralTextures::makeRoughness(64, 0.92f);

    auto coralAlbedo = ProceduralTextures::makeCoralAlbedo();
    auto coralNormal = ProceduralTextures::makeCoralNormal();
    auto coralMetal = ProceduralTextures::makeMetallic(64, 0.12f);
    auto coralRough = ProceduralTextures::makeRoughness(64, 0.55f);

    auto kelpAlbedo = ProceduralTextures::makeKelpAlbedo();
    auto kelpFlatNormal = ProceduralTextures::makeFlatNormal();
    auto kelpMetal = ProceduralTextures::makeMetallic(64, 0.02f);
    auto kelpRough = ProceduralTextures::makeRoughness(64, 0.7f);

    auto skyCube = ProceduralTextures::makeUnderwaterSky();
    Mesh skyboxMesh = makeSkyboxCube();
    const float kSeabedSize = 80.0f;
    const float kSeabedHalf = kSeabedSize * 0.5f;
    Mesh seabed = Geometry::makePlane(kSeabedSize, kSeabedSize, 32);
    const glm::mat4 floorTransform(1.0f);

    Mesh kelpMesh = Geometry::makeKelpSegment();

    
    std::vector<DrawObject> corals; // corals arranged in a ring with a forward offset; radius varies per index (patterned spacing)
    for (int i = 0; i < 44; ++i) {
        const float angle = i * 0.55f;
        const float radius = 6.0f + (i % 5) * 2.2f;
        DrawObject c;
        c.mesh = Geometry::makeLowPolyCoral(0.8f + (i % 3) * 0.25f, 1000 + i);
        c.transform = glm::translate(glm::mat4(1.0f), glm::vec3(std::cos(angle) * radius, 0.0f, -12.0f - std::sin(angle) * radius * 0.5f));
        c.transform = glm::rotate(c.transform, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        c.albedoTint = glm::vec3(0.9f + (i % 3) * 0.05f, 0.55f + (i % 4) * 0.08f, 0.5f);
        c.metallic = 0.1f;
        c.roughness = 0.5f;
        corals.push_back(std::move(c));
    }

    std::vector<DrawObject> rocks; // rocks placed in a linear pattern
    for (int i = 0; i < 14; ++i) {
        DrawObject r;
        r.mesh = Geometry::makeRock(0.5f + (i % 3) * 0.2f, 2000 + i);
        r.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-8.0f + i * 1.7f, 0.0f, -18.0f - (i % 4) * 2.5f));
        r.transform = glm::rotate(r.transform, static_cast<float>(i) * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f));
        r.albedoTint = glm::vec3(0.55f, 0.5f, 0.48f);
        r.metallic = 0.03f;
        r.roughness = 0.95f;
        rocks.push_back(std::move(r));
    }

    const glm::vec3 kKelpColorDark{0.10f, 0.45f, 0.18f};
    const glm::vec3 kKelpColorBright{0.32f, 0.90f, 0.42f};

    struct KelpInstance {
        glm::mat4 transform;
        glm::vec3 color;
        float swayPhase;
        glm::vec3 swayAxis;
    };

    struct PtfKelpPlant {
        Mesh mesh;
        glm::vec3 color;
        float swayPhase;
        glm::vec3 swayAxis;
    };

    std::vector<PtfKelpPlant> ptfKelpPlants;
    std::vector<KelpInstance> kelps;

    const std::vector<std::vector<glm::vec3>> kelpSplineDefs = {
        {{-16.0f, 0.0f, -34.0f}, {-10.0f, 2.0f, -31.0f}, {-3.0f, 4.2f, -29.0f}, {4.0f, 5.5f, -31.0f},
         {11.0f, 4.0f, -34.0f}, {18.0f, 2.2f, -37.0f}, {24.0f, 0.5f, -40.0f}},
        {{-10.0f, 0.0f, -35.0f}, {-4.0f, 2.2f, -32.0f}, {3.0f, 4.5f, -30.0f}, {10.0f, 5.8f, -32.0f},
         {17.0f, 4.2f, -35.0f}, {23.0f, 2.4f, -38.0f}, {29.0f, 0.6f, -41.0f}},
        {{-22.0f, 0.0f, -35.0f}, {-16.0f, 1.8f, -32.0f}, {-9.0f, 3.8f, -30.0f}, {-2.0f, 5.0f, -32.0f},
         {5.0f, 3.6f, -35.0f}, {12.0f, 1.9f, -38.0f}, {18.0f, 0.4f, -41.0f}},
    };

    {
        std::mt19937 rng(4242u);
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        for (const auto& controlPoints : kelpSplineDefs) {
            ParallelTransportSpline kelpPath;
            kelpPath.controlPoints = controlPoints;
            kelpPath.samplesPerSegment = 16;
            kelpPath.rebuild(glm::vec3(0.0f, 0.0f, 1.0f));

            PtfKelpPlant plant;
            plant.mesh = Geometry::makeKelpAlongSpline(kelpPath, 0.09f);
            plant.color = glm::mix(kKelpColorDark, kKelpColorBright, u(rng));
            plant.swayPhase = u(rng) * glm::two_pi<float>();
            plant.swayAxis = kelpPath.frameAt(1.0f).binormal;
            ptfKelpPlants.push_back(std::move(plant));
        }
    }

    const int kKelpCount = 48;
    const unsigned int kKelpSeed = 1337u;
    const float kKelpEdgeMargin = 2.0f;
    const glm::vec2 kKelpAreaMin{-kSeabedHalf + kKelpEdgeMargin, -kSeabedHalf + kKelpEdgeMargin};
    const glm::vec2 kKelpAreaMax{kSeabedHalf - kKelpEdgeMargin, -8.0f};
    const glm::vec2 kKelpScaleRange{0.9f, 2.6f};
    const glm::vec2 kKelpClearCenter{0.0f, -20.0f};
    const float kKelpClearRadius = 6.0f;
    {
        std::mt19937 rng(kKelpSeed);
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        for (int i = 0; i < kKelpCount; ++i) {
            glm::vec2 xz;
            for (int attempt = 0; attempt < 8; ++attempt) {
                xz = glm::vec2(glm::mix(kKelpAreaMin.x, kKelpAreaMax.x, u(rng)),
                               glm::mix(kKelpAreaMin.y, kKelpAreaMax.y, u(rng)));
                if (glm::length(xz - kKelpClearCenter) >= kKelpClearRadius) break;
            }
            const float limit = kSeabedHalf - kKelpEdgeMargin;
            xz = glm::clamp(xz, glm::vec2(-limit), glm::vec2(limit));
            const float yaw = u(rng) * glm::two_pi<float>();
            const float scale = glm::mix(kKelpScaleRange.x, kKelpScaleRange.y, u(rng));
            const glm::vec3 color = glm::mix(kKelpColorDark, kKelpColorBright, u(rng));

            KelpInstance inst;
            inst.transform = glm::translate(glm::mat4(1.0f), glm::vec3(xz.x, 0.0f, xz.y)) *
                             glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
                             glm::scale(glm::mat4(1.0f), glm::vec3(scale));
            inst.color = color;
            inst.swayPhase = u(rng) * glm::two_pi<float>();
            inst.swayAxis = glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw));
            kelps.push_back(inst);
        }
    }

    ShadowMap shadow;
    shadow.create();

    const glm::mat4 identityTransform(1.0f);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        const float time = static_cast<float>(glfwGetTime());
        const float dt = time - lastFrame;
        lastFrame = time;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        gCamera.processKeyboard(gKeys[GLFW_KEY_W], gKeys[GLFW_KEY_S], gKeys[GLFW_KEY_A], gKeys[GLFW_KEY_D],
                                gKeys[GLFW_KEY_SPACE], gKeys[GLFW_KEY_LEFT_SHIFT], dt);

        const glm::vec3 camPos = gCamera.position;
        const glm::mat4 proj = gCamera.projectionMatrix(static_cast<float>(gWidth) / static_cast<float>(gHeight));
        const glm::mat4 view = gCamera.viewMatrix();

        constexpr int kNumLights = 1;
        glm::vec3 lightPos[kNumLights];
        glm::vec3 lightCol[kNumLights] = {glm::vec3(10.0f, 12.0f, 8.0f)};
        float lightRad[kNumLights] = {50.0f};
        lightPos[0] = glm::vec3(25.0f, 14.0f, 5.0f);
        const glm::vec3 lightDir = glm::normalize(lightPos[0] - glm::vec3(0.0f, 0.0f, -25.0f));
        const float orthoSize = 45.0f;
        const glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 70.0f);
        const glm::mat4 lightView =
            glm::lookAt(lightPos[0] - lightDir * 5.0f, glm::vec3(0.0f, 0.0f, -25.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, shadow.size, shadow.size);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shadowShader.setBool("useSkin", false);

        auto shadowDraw = [&](const Mesh& mesh, const glm::mat4& model) {
            shadowShader.setMat4("model", model);
            mesh.draw();
        };

        shadowDraw(seabed, floorTransform);
        for (const auto& c : corals) shadowDraw(c.mesh, c.transform);
        for (const auto& r : rocks) shadowDraw(r.mesh, r.transform);

        glDisable(GL_CULL_FACE);
        shadowShader.setBool("useKelpSway", false);
        for (const auto& plant : ptfKelpPlants) {
            shadowShader.setBool("useKelpSway", true);
            shadowShader.setFloat("kelpTime", time);
            shadowShader.setFloat("kelpSwayPhase", plant.swayPhase);
            shadowShader.setVec3("kelpSwayAxis", plant.swayAxis);
            shadowDraw(plant.mesh, identityTransform);
        }
        for (const auto& kelp : kelps) {
            shadowShader.setBool("useKelpSway", true);
            shadowShader.setFloat("kelpTime", time);
            shadowShader.setFloat("kelpSwayPhase", kelp.swayPhase);
            shadowShader.setVec3("kelpSwayAxis", kelp.swayAxis);
            shadowDraw(kelpMesh, kelp.transform);
        }
        shadowShader.setBool("useKelpSway", false);
        glEnable(GL_CULL_FACE);

        glViewport(0, 0, gWidth, gHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.01f, 0.03f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthFunc(GL_LEQUAL);
        skyShader.use();
        skyShader.setMat4("projection", proj);
        skyShader.setMat4("view", view);
        skyShader.setFloat("depthTint", 0.65f);
        skyCube.bind(0);
        skyShader.setInt("skybox", 0);
        skyboxMesh.draw();
        glDepthFunc(GL_LESS);

        pbrShader.use();
        pbrShader.setMat4("view", view);
        pbrShader.setMat4("projection", proj);
        pbrShader.setVec3("camPos", camPos);
        pbrShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        pbrShader.setInt("albedoMap", 0);
        pbrShader.setInt("normalMap", 1);
        pbrShader.setInt("metallicMap", 2);
        pbrShader.setInt("roughnessMap", 3);
        pbrShader.setInt("shadowMap", 4);
        pbrShader.setInt("numLights", kNumLights);
        pbrShader.setFloat("underwaterFogDensity", 0.018f);
        pbrShader.setBool("useNormalMap", true);
        pbrShader.setBool("useSkin", false);
        pbrShader.setBool("useKelpSway", false);
        for (int i = 0; i < kNumLights; ++i) {
            const std::string idx = std::to_string(i);
            pbrShader.setVec3(("lightPositions[" + idx + "]").c_str(), lightPos[i]);
            pbrShader.setVec3(("lightColors[" + idx + "]").c_str(), lightCol[i]);
            pbrShader.setFloat(("lightRadii[" + idx + "]").c_str(), lightRad[i]);
        }
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, shadow.depthMap);

        drawPBR(pbrShader, seabed, floorTransform, glm::vec3(1.0f), 0.02f, 0.92f, sandAlbedo, sandNormal, sandMetal,
                sandRough);
        for (const auto& c : corals)
            drawPBR(pbrShader, c.mesh, c.transform, c.albedoTint, c.metallic, c.roughness, coralAlbedo, coralNormal,
                    coralMetal, coralRough);
        for (const auto& r : rocks)
            drawPBR(pbrShader, r.mesh, r.transform, r.albedoTint, r.metallic, r.roughness, sandAlbedo, sandNormal,
                    sandMetal, sandRough);

        auto drawKelp = [&](const Mesh& mesh, const glm::mat4& model, const glm::vec3& color, float swayPhase,
                            const glm::vec3& swayAxis) {
            pbrShader.setBool("useKelpSway", true);
            pbrShader.setFloat("kelpTime", time);
            pbrShader.setFloat("kelpSwayPhase", swayPhase);
            pbrShader.setVec3("kelpSwayAxis", swayAxis);
            drawPBR(pbrShader, mesh, model, color, 0.02f, 0.7f, kelpAlbedo, kelpFlatNormal, kelpMetal, kelpRough);
            pbrShader.setBool("useKelpSway", false);
        };

        glDisable(GL_CULL_FACE);
        for (const auto& plant : ptfKelpPlants)
            drawKelp(plant.mesh, identityTransform, plant.color, plant.swayPhase, plant.swayAxis);
        for (const auto& kelp : kelps) drawKelp(kelpMesh, kelp.transform, kelp.color, kelp.swayPhase, kelp.swayAxis);
        glEnable(GL_CULL_FACE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shadow.destroy();
    glfwTerminate();
    return 0;
}
