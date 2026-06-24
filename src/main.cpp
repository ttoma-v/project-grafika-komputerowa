#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Anglerfish.h"
#include "Camera.h"
#include "Geometry.h"
#include "GlLoader.h"
#include "ModelLoader.h"
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
static bool gHeadlightsOn = true;
static bool gShadowsEnabled = true;
static float gFogDensity = 0.012f;
static float gDistortionStrength = 0.003f;

#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif

static float seabedHeightAt(float x, float z) {
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
    if (key == GLFW_KEY_F && action == GLFW_PRESS) gHeadlightsOn = !gHeadlightsOn;
    if (key == GLFW_KEY_T && action == GLFW_PRESS) gShadowsEnabled = !gShadowsEnabled;
    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) gFogDensity = glm::max(0.005f, gFogDensity - 0.003f);
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) gFogDensity = glm::min(0.05f, gFogDensity + 0.003f);
    if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        if (gDistortionStrength < 0.001f) gDistortionStrength = 0.003f;
        else if (gDistortionStrength < 0.004f) gDistortionStrength = 0.006f;
        else gDistortionStrength = 0.0f;
    }
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

static Mesh makeScreenQuad() {
    std::vector<Vertex> verts = {
        {{-1, -1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0}},
        {{1, -1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0}},
        {{1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1}},
        {{-1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1}},
    };
    std::vector<unsigned int> inds = {0, 1, 2, 0, 2, 3};
    Mesh m;
    m.upload(verts, inds);
    return m;
}

struct Framebuffer {
    unsigned int fbo = 0;
    unsigned int colorTex = 0;
    unsigned int depthTex = 0;
    int w = 0;
    int h = 0;

    void create(int width, int height) {
        w = width;
        h = height;
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &colorTex);
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

        glGenTextures(1, &depthTex);
        glBindTexture(GL_TEXTURE_2D, depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer incomplete\n";
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void destroy() {
        if (colorTex) glDeleteTextures(1, &colorTex);
        if (depthTex) glDeleteTextures(1, &depthTex);
        if (fbo) glDeleteFramebuffers(1, &fbo);
        colorTex = depthTex = fbo = 0;
    }
};

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
                    const Texture2D& rough, const glm::vec3& emissive = glm::vec3(0.0f),
                    const Texture2D* flow = nullptr) {
    shader.setMat4("model", model);
    shader.setVec3("materialAlbedoTint", tint);
    shader.setFloat("materialMetallic", metallic);
    shader.setFloat("materialRoughness", roughness);
    shader.setVec3("materialEmissive", emissive);
    albedo.bind(0);
    normal.bind(1);
    metal.bind(2);
    rough.bind(3);
    if (flow && flow->id) flow->bind(5);
    mesh.draw();
}

static const Texture2D& submeshAlbedo(const ModelLoader::SubMesh& sub, const ModelLoader::GltfModel& model) {
    return sub.albedo.id ? sub.albedo : model.whiteAlbedo;
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

    int launchWidth = gWidth;
    int launchHeight = gHeight;
    int launchPosX = 80;
    int launchPosY = 60;
    if (GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor()) {
        if (const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor)) {
            // Border-visible near-fullscreen window: large enough to fill view,
            // but still decorated to avoid exclusive fullscreen issues.
            launchWidth = glm::max(960, mode->width - 80);
            launchHeight = glm::max(540, mode->height - 80);
            launchPosX = (mode->width - launchWidth) / 2;
            launchPosY = (mode->height - launchHeight) / 2;
        }
    }

    GLFWwindow* window = glfwCreateWindow(launchWidth, launchHeight, "Bathyscaphe - Deep Underwater", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwGetFramebufferSize(window, &gWidth, &gHeight);
    glfwSetWindowPos(window, launchPosX, launchPosY);
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
    Shader screenShader;
Shader particlesShader;
    const std::string skyVert = assetPath("shaders/skybox.vert");
    const std::string skyFrag = assetPath("shaders/skybox.frag");
    const std::string shadowVert = assetPath("shaders/shadow_depth.vert");
    const std::string shadowFrag = assetPath("shaders/shadow_depth.frag");
    const std::string pbrVert = assetPath("shaders/pbr.vert");
    const std::string pbrFrag = assetPath("shaders/pbr.frag");
    const std::string screenVert = assetPath("shaders/screen.vert");
    const std::string screenFrag = assetPath("shaders/screen.frag");
    const std::string particlesVert = assetPath("shaders/particles.vert");
    const std::string particlesFrag = assetPath("shaders/particles.frag");

    if (!shadowShader.loadFromFiles(shadowVert, shadowFrag) || !skyShader.loadFromFiles(skyVert, skyFrag) ||
        !pbrShader.loadFromFiles(pbrVert, pbrFrag) || !screenShader.loadFromFiles(screenVert, screenFrag) ||
        !particlesShader.loadFromFiles(particlesVert, particlesFrag)) {
        std::cerr << "Shader load failed. Run exe from build/Release or ensure assets/ is nearby.\n";
        std::cerr << "Tried: " << pbrVert << '\n';
        return 1;
    }
    std::cout << "Shaders OK. Assets from: " << fs::current_path() << '\n';

    auto sandAlbedo = Texture2D::loadFromFile(assetPath("textures/sand_color.jpg"));
    auto sandNormal = Texture2D::loadFromFile(assetPath("textures/sand_normal.jpg"));
    auto sandRough = Texture2D::loadFromFile(assetPath("textures/sand_roughness.jpg"));
    auto sandMetal = ProceduralTextures::makeMetallic(64, 0.02f);
    if (!sandAlbedo.id || !sandNormal.id || !sandRough.id) {
        std::cerr << "Sand texture load failed. Ensure assets/textures/ contains sand_*.jpg\n";
        return 1;
    }

    auto coralAlbedo = ProceduralTextures::makeCoralAlbedo();
    auto coralNormal = ProceduralTextures::makeCoralNormal();
    auto coralMetal = ProceduralTextures::makeMetallic(64, 0.12f);
    auto coralRough = ProceduralTextures::makeRoughness(64, 0.55f);

    auto kelpAlbedo = ProceduralTextures::makeKelpAlbedo();
    auto kelpFlatNormal = ProceduralTextures::makeFlatNormal();
    auto kelpMetal = ProceduralTextures::makeMetallic(64, 0.02f);
    auto kelpRough = ProceduralTextures::makeRoughness(64, 0.7f);

    auto flowMap = ProceduralTextures::makeFlowMap();
    auto skyCube = ProceduralTextures::makeUnderwaterSky();
    Mesh skyboxMesh = makeSkyboxCube();
    Mesh screenQuad = makeScreenQuad();
    const float kSeabedSize = 100.0f;
    const float kSeabedHalf = kSeabedSize * 0.5f;
    Mesh seabed = Geometry::makeSeabed(kSeabedSize, kSeabedSize, 32);
    const glm::mat4 floorTransform(1.0f);

    const glm::vec3 anglerfishCircleCenter{0.0f, 1.6f, -20.0f};

    Anglerfish anglerfish;
    anglerfish.load(assetPath("Anglerfish.glb"), 2.0f);
    anglerfish.setupCircularPath(anglerfishCircleCenter, 8.0f, 8);

    Anglerfish piranha;
    piranha.load(assetPath("Piranha.glb"), 1.0f);
    piranha.setupCircularPath(glm::vec3(12.0f, 3.0f, -12.0f), 6.0f, 6);

    Anglerfish chest, anchor, barrel, urchin;
    chest.load(assetPath("Chest.glb"), 1.5f);
    anchor.load(assetPath("Anchor.glb"), 1.5f);
    barrel.load(assetPath("Barrel.glb"), 1.2f);
    urchin.load(assetPath("Urchin.glb"), 0.8f);

    constexpr float kCastleTargetSize = 66.0f;
    const float castleX = 15.0f;
    const float castleZ = -40.0f;
    ModelLoader::GltfModel castleChurch;
    if (!ModelLoader::loadGlb(assetPath("castle_church.glb"), castleChurch, kCastleTargetSize)) {
        std::cerr << "Failed to load castle_church.glb. Ensure assets/castle_church.glb exists.\n";
        return 1;
    }

    const float castleY = seabedHeightAt(castleX, castleZ) + kCastleTargetSize * 0.5f;
    const glm::mat4 castleChurchTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(castleX, castleY, castleZ)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(-20.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::mat4 chestTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.7f, -13.0f)) *
                                     glm::rotate(glm::mat4(1.0f), glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 anchorTransform = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.3f, -14.0f)) *
                                      glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 barrelTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, 0.6f, -14.5f)) *
                                      glm::rotate(glm::mat4(1.0f), glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                                      glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 urchinTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.2f, -11.5f));

    auto uploadJoints = [](const Shader& sh, const std::vector<glm::mat4>& mats) {
        for (size_t i = 0; i < mats.size(); ++i) {
            sh.setMat4(("jointMatrices[" + std::to_string(i) + "]").c_str(), mats[i]);
        }
    };

    Mesh kelpMesh = Geometry::makeKelpSegment();

    std::vector<DrawObject> corals;
    {
        std::mt19937 rng(9001u);
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        const int kCoralCount = 120;
        for (int i = 0; i < kCoralCount; ++i) {
            glm::vec2 xz;
            for (int attempt = 0; attempt < 10; ++attempt) {
                xz = glm::vec2(glm::mix(-kSeabedHalf + 2.5f, kSeabedHalf - 2.5f, u(rng)),
                               glm::mix(-kSeabedHalf + 2.5f, kSeabedHalf - 2.5f, u(rng)));
                if (glm::length(xz) >= 4.5f) break;
            }

            const float yaw = u(rng) * glm::two_pi<float>();
            const float scale = glm::mix(0.8f, 1.35f, u(rng));
            DrawObject c;
            c.mesh = Geometry::makeLowPolyCoral(scale, 1000 + i);
            c.transform = glm::translate(glm::mat4(1.0f), glm::vec3(xz.x, 0.0f, xz.y));
            c.transform = glm::rotate(c.transform, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            c.albedoTint = glm::vec3(0.84f + u(rng) * 0.24f, 0.5f + u(rng) * 0.28f, 0.45f + u(rng) * 0.12f);
            c.metallic = 0.1f;
            c.roughness = 0.5f;
            corals.push_back(std::move(c));
        }
    }

    std::vector<DrawObject> rocks;
    {
        std::mt19937 rng(13331u);
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        const int kRockCount = 64;
        for (int i = 0; i < kRockCount; ++i) {
            glm::vec2 xz;
            for (int attempt = 0; attempt < 10; ++attempt) {
                xz = glm::vec2(glm::mix(-kSeabedHalf + 2.0f, kSeabedHalf - 2.0f, u(rng)),
                               glm::mix(-kSeabedHalf + 2.0f, kSeabedHalf - 2.0f, u(rng)));
                if (glm::length(xz) >= 3.8f) break;
            }

            DrawObject r;
            r.mesh = Geometry::makeRock(glm::mix(0.45f, 0.95f, u(rng)), 2000 + i);
            r.transform = glm::translate(glm::mat4(1.0f), glm::vec3(xz.x, 0.0f, xz.y));
            r.transform = glm::rotate(r.transform, u(rng) * glm::two_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
            r.albedoTint = glm::vec3(0.48f + u(rng) * 0.14f, 0.45f + u(rng) * 0.12f, 0.43f + u(rng) * 0.1f);
            r.metallic = 0.03f;
            r.roughness = 0.95f;
            rocks.push_back(std::move(r));
        }
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

    const int kKelpCount = 96;
    const unsigned int kKelpSeed = 1337u;
    const float kKelpEdgeMargin = 2.0f;
    const glm::vec2 kKelpAreaMin{-kSeabedHalf + kKelpEdgeMargin, -kSeabedHalf + kKelpEdgeMargin};
    const glm::vec2 kKelpAreaMax{kSeabedHalf - kKelpEdgeMargin, kSeabedHalf - kKelpEdgeMargin};
    const glm::vec2 kKelpScaleRange{0.8f, 2.2f};
    const glm::vec2 kKelpClearCenter{0.0f, 0.0f};
    const float kKelpClearRadius = 4.5f;
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

    ShadowMap fishShadowMap;
    fishShadowMap.create();
    ShadowMap cameraShadowMap;
    cameraShadowMap.create();

    Framebuffer sceneFbo;
    sceneFbo.create(gWidth, gHeight);

    std::vector<glm::vec3> dustParticles;
    {
        std::mt19937 rng(777u);
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        const int kDustCount = 2200;
        for (int i = 0; i < kDustCount; ++i) {
            float x = glm::mix(-kSeabedHalf + 1.0f, kSeabedHalf - 1.0f, u(rng));
            float z = glm::mix(-kSeabedHalf + 1.0f, kSeabedHalf - 1.0f, u(rng));

            float h01 = std::pow(u(rng), 1.8f);
            float y = seabedHeightAt(x, z) + glm::mix(0.35f, 9.0f, h01);

            dustParticles.emplace_back(x, y, z);
        }
    }

    unsigned int dustVbo = 0;
    unsigned int dustVao = 0;
    glGenVertexArrays(1, &dustVao);
    glGenBuffers(1, &dustVbo);
    glBindVertexArray(dustVao);
    glBindBuffer(GL_ARRAY_BUFFER, dustVbo);
    glBufferData(GL_ARRAY_BUFFER, dustParticles.size() * sizeof(glm::vec3), dustParticles.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glBindVertexArray(0);

    glEnable(0x8642); // GL_PROGRAM_POINT_SIZE

    const glm::mat4 identityTransform(1.0f);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        const float time = static_cast<float>(glfwGetTime());
        const float dt = time - lastFrame;
        lastFrame = time;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        gCamera.processKeyboard(gKeys[GLFW_KEY_W], gKeys[GLFW_KEY_S], gKeys[GLFW_KEY_A], gKeys[GLFW_KEY_D],
                                gKeys[GLFW_KEY_SPACE], gKeys[GLFW_KEY_LEFT_SHIFT], dt);
        const float cameraClearance = 0.55f;
        const float floorY = seabedHeightAt(gCamera.position.x, gCamera.position.z) + cameraClearance;
        if (gCamera.position.y < floorY) gCamera.position.y = floorY;

        const glm::vec3 fwd = gCamera.forward();
        const glm::vec3 right = gCamera.right();
        const glm::vec3 up = gCamera.up();
        const glm::vec3 camPos = gCamera.position;
        const float safeAspect = static_cast<float>(gWidth) / static_cast<float>(glm::max(gHeight, 1));
        const glm::mat4 proj = gCamera.projectionMatrix(safeAspect);
        const glm::mat4 view = gCamera.viewMatrix();

        anglerfish.update(time);
        const glm::mat4 anglerfishTransform = anglerfish.transform();
        piranha.update(time * 2.0f);
        const glm::mat4 piranhaTransform = piranha.transform();

        constexpr int kNumLights = 3;
        glm::vec3 lightPos[kNumLights];
        glm::vec3 lightDirWs[kNumLights];
        glm::vec3 lightCol[kNumLights] = {
            glm::vec3(1.25f, 1.20f, 1.08f),
            glm::vec3(1.25f, 1.20f, 1.08f),
            glm::vec3(0.35f, 1.1f, 1.4f),
        };
        float lightRad[kNumLights] = {30.0f, 30.0f, 7.0f};

        if (!gHeadlightsOn) {
            lightCol[0] = glm::vec3(0.0f);
            lightCol[1] = glm::vec3(0.0f);
        }

        lightPos[0] = camPos + fwd * 1.7f + up * -0.12f + right * -0.42f;
        lightPos[1] = camPos + fwd * 1.7f + up * -0.12f + right * 0.42f;
        lightPos[0] += right * std::sin(time * 2.1f) * 0.04f;
        lightPos[1] += right * std::sin(time * 2.3f + 1.0f) * 0.04f;
        lightPos[2] = glm::vec3(0.0f, 1.6f, -20.0f);
        if (anglerfish.valid() && anglerfish.hasLure()) {
            lightPos[2] = anglerfish.lureLightPosition();
        }
        lightDirWs[0] = fwd;
        lightDirWs[1] = fwd;
        lightDirWs[2] = glm::vec3(0.0f, -1.0f, 0.0f);
        lightCol[2] *= 0.55f + 0.15f * std::sin(time * 3.0f);

        // Fish shadow map (from anglerfish lure light)
        const glm::vec3 lureDir = glm::normalize(glm::vec3(0.0f, -1.0f, -0.55f));
        const glm::vec3 lureTarget = lightPos[2] + lureDir * 12.0f;
        const glm::mat4 fishLightProjection = glm::perspective(glm::radians(85.0f), 1.0f, 0.35f, 45.0f);
        const glm::mat4 fishLightView = glm::lookAt(lightPos[2], lureTarget, glm::vec3(0.0f, 0.0f, 1.0f));
        const glm::mat4 fishLightSpaceMatrix = fishLightProjection * fishLightView;

        // Camera shadow map (from camera forward direction)
        const glm::vec3 camShadowPos = camPos + fwd * 1.6f + up * -0.1f;
        const glm::vec3 camShadowTarget = camShadowPos + fwd * 14.0f;
        const glm::mat4 camLightProjection = glm::perspective(glm::radians(70.0f), 1.0f, 0.35f, 48.0f);
        const glm::mat4 camLightView = glm::lookAt(camShadowPos, camShadowTarget, up);
        const glm::mat4 camLightSpaceMatrix = camLightProjection * camLightView;

        auto renderShadowPass = [&](const ShadowMap& map, const glm::mat4& lightSpaceMatrix) {
            glViewport(0, 0, map.size, map.size);
            glBindFramebuffer(GL_FRAMEBUFFER, map.fbo);
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

            if (anglerfish.valid()) {
                shadowShader.setMat4("model", anglerfishTransform);
                shadowShader.setBool("useSkin", true);
                uploadJoints(shadowShader, anglerfish.joints());
                for (const auto& sub : anglerfish.submeshes()) sub.mesh.draw();
                shadowShader.setBool("useSkin", false);
            }

            if (piranha.valid()) {
                shadowShader.setMat4("model", piranhaTransform);
                shadowShader.setBool("useSkin", true);
                uploadJoints(shadowShader, piranha.joints());
                for (const auto& sub : piranha.submeshes()) sub.mesh.draw();
                shadowShader.setBool("useSkin", false);
            }

            auto drawStaticShadow = [&](const Anglerfish& obj, const glm::mat4& trans) {
                if (!obj.valid()) return;
                shadowShader.setMat4("model", trans);
                shadowShader.setBool("useSkin", false);
                for (const auto& sub : obj.submeshes()) sub.mesh.draw();
            };
            if (castleChurch.valid) {
                shadowShader.setMat4("model", castleChurchTransform);
                shadowShader.setBool("useSkin", false);
                for (const auto& sub : castleChurch.submeshes) sub.mesh.draw();
            }
            drawStaticShadow(chest, chestTransform);
            drawStaticShadow(anchor, anchorTransform);
            drawStaticShadow(barrel, barrelTransform);
            drawStaticShadow(urchin, urchinTransform);

            glEnable(GL_CULL_FACE);
        };
        renderShadowPass(fishShadowMap, fishLightSpaceMatrix);
        renderShadowPass(cameraShadowMap, camLightSpaceMatrix);

        glViewport(0, 0, gWidth, gHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo.fbo);
        glClearColor(0.06f, 0.21f, 0.23f, 1.0f);
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
        pbrShader.setMat4("fishLightSpaceMatrix", fishLightSpaceMatrix);
        pbrShader.setMat4("camLightSpaceMatrix", camLightSpaceMatrix);
        pbrShader.setInt("albedoMap", 0);
        pbrShader.setInt("normalMap", 1);
        pbrShader.setInt("metallicMap", 2);
        pbrShader.setInt("roughnessMap", 3);
        pbrShader.setInt("shadowMapFish", 4);
        pbrShader.setInt("shadowMapCam", 6);
        pbrShader.setInt("flowMap", 5);
        pbrShader.setInt("numLights", kNumLights);
        pbrShader.setFloat("underwaterFogDensity", gFogDensity);
        pbrShader.setBool("shadowsEnabled", gShadowsEnabled);
        pbrShader.setBool("useNormalMap", true);
        pbrShader.setBool("useSkin", false);
        pbrShader.setBool("useKelpSway", false);
        for (int i = 0; i < kNumLights; ++i) {
            const std::string idx = std::to_string(i);
            pbrShader.setVec3(("lightPositions[" + idx + "]").c_str(), lightPos[i]);
            pbrShader.setVec3(("lightDirections[" + idx + "]").c_str(), lightDirWs[i]);
            pbrShader.setVec3(("lightColors[" + idx + "]").c_str(), lightCol[i]);
            pbrShader.setFloat(("lightRadii[" + idx + "]").c_str(), lightRad[i]);
        }
        pbrShader.setFloat("headlightCosCutoff", std::cos(glm::radians(16.0f)));
        pbrShader.setFloat("headlightSoftCosCutoff", std::cos(glm::radians(24.0f)));
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, fishShadowMap.depthMap);
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_2D, cameraShadowMap.depthMap);

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

        pbrShader.setBool("useNormalMap", false);
        if (anglerfish.valid()) {
            const ModelLoader::GltfModel& anglerfishModel = anglerfish.model();
            pbrShader.setBool("useSkin", true);
            uploadJoints(pbrShader, anglerfish.joints());
            for (const auto& sub : anglerfish.submeshes()) {
                drawPBR(pbrShader, sub.mesh, anglerfishTransform, sub.baseColorFactor, sub.metallicFactor,
                          sub.roughnessFactor, submeshAlbedo(sub, anglerfishModel), anglerfishModel.flatNormal,
                          anglerfishModel.whiteScalar, anglerfishModel.whiteScalar, sub.emissive);
            }
            pbrShader.setBool("useSkin", false);
        }

        if (piranha.valid()) {
            const ModelLoader::GltfModel& piranhaModel = piranha.model();
            pbrShader.setBool("useSkin", true);
            uploadJoints(pbrShader, piranha.joints());
            for (const auto& sub : piranha.submeshes()) {
                drawPBR(pbrShader, sub.mesh, piranhaTransform, sub.baseColorFactor, sub.metallicFactor,
                          sub.roughnessFactor, submeshAlbedo(sub, piranhaModel), piranhaModel.flatNormal,
                          piranhaModel.whiteScalar, piranhaModel.whiteScalar, sub.emissive);
            }
            pbrShader.setBool("useSkin", false);
        }

        auto drawStaticPBR = [&](const Anglerfish& obj, const glm::mat4& trans) {
            if (!obj.valid()) return;
            const ModelLoader::GltfModel& mod = obj.model();
            pbrShader.setBool("useSkin", false);
            for (const auto& sub : obj.submeshes()) {
                drawPBR(pbrShader, sub.mesh, trans, sub.baseColorFactor, sub.metallicFactor, sub.roughnessFactor,
                        submeshAlbedo(sub, mod), mod.flatNormal, mod.whiteScalar, mod.whiteScalar, sub.emissive);
            }
        };
        if (castleChurch.valid) {
            pbrShader.setBool("useSkin", false);
            for (const auto& sub : castleChurch.submeshes) {
                drawPBR(pbrShader, sub.mesh, castleChurchTransform, sub.baseColorFactor, sub.metallicFactor,
                        sub.roughnessFactor, submeshAlbedo(sub, castleChurch), castleChurch.flatNormal,
                        castleChurch.whiteScalar, castleChurch.whiteScalar, sub.emissive);
            }
        }
        drawStaticPBR(chest, chestTransform);
        drawStaticPBR(anchor, anchorTransform);
        drawStaticPBR(barrel, barrelTransform);
        drawStaticPBR(urchin, urchinTransform);
        pbrShader.setBool("useNormalMap", true);

        // Dust particles in water
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        particlesShader.use();
        particlesShader.setMat4("view", view);
        particlesShader.setMat4("projection", proj);
        particlesShader.setFloat("time", time);
        glBindVertexArray(dustVao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(dustParticles.size()));
        glBindVertexArray(0);
        glDisable(GL_BLEND);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, gWidth, gHeight);
        glClearColor(0.06f, 0.21f, 0.23f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        screenShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneFbo.colorTex);
        flowMap.bind(1);
        screenShader.setInt("sceneColor", 0);
        screenShader.setInt("flowMap", 1);
        screenShader.setFloat("time", time);
        screenShader.setFloat("distortionStrength", gDistortionStrength);
        screenQuad.draw();
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    sceneFbo.destroy();
    fishShadowMap.destroy();
    cameraShadowMap.destroy();
    if (dustVao) glDeleteVertexArrays(1, &dustVao);
    if (dustVbo) glDeleteBuffers(1, &dustVbo);
    glfwTerminate();
    return 0;
}
