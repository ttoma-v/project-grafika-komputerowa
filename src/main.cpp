#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Geometry.h"
#include "GlLoader.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>
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
    Shader pbrShader;
    const std::string skyVert = assetPath("shaders/skybox.vert");
    const std::string skyFrag = assetPath("shaders/skybox.frag");
    const std::string pbrVert = assetPath("shaders/pbr.vert");
    const std::string pbrFrag = assetPath("shaders/pbr.frag");

    if (!skyShader.loadFromFiles(skyVert, skyFrag) || !pbrShader.loadFromFiles(pbrVert, pbrFrag)) {
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

    auto skyCube = ProceduralTextures::makeUnderwaterSky();
    Mesh skyboxMesh = makeSkyboxCube();
    const float kSeabedSize = 80.0f;
    Mesh seabed = Geometry::makePlane(kSeabedSize, kSeabedSize, 32);
    const glm::mat4 floorTransform(1.0f);

    
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
        glm::vec3 lightCol[kNumLights] = {glm::vec3(4.0f, 4.2f, 3.6f)};
        float lightRad[kNumLights] = {50.0f};
        lightPos[0] = glm::vec3(std::sin(time * 0.4f) * 12.0f, 10.0f, -18.0f + std::cos(time * 0.35f) * 8.0f);

        glViewport(0, 0, gWidth, gHeight);
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
        pbrShader.setInt("albedoMap", 0);
        pbrShader.setInt("normalMap", 1);
        pbrShader.setInt("metallicMap", 2);
        pbrShader.setInt("roughnessMap", 3);
        pbrShader.setInt("numLights", kNumLights);
        pbrShader.setFloat("underwaterFogDensity", 0.018f);
        pbrShader.setBool("useNormalMap", true);
        pbrShader.setBool("useSkin", false);
        for (int i = 0; i < kNumLights; ++i) {
            const std::string idx = std::to_string(i);
            pbrShader.setVec3(("lightPositions[" + idx + "]").c_str(), lightPos[i]);
            pbrShader.setVec3(("lightColors[" + idx + "]").c_str(), lightCol[i]);
            pbrShader.setFloat(("lightRadii[" + idx + "]").c_str(), lightRad[i]);
        }

        drawPBR(pbrShader, seabed, floorTransform, glm::vec3(1.0f), 0.02f, 0.92f, sandAlbedo, sandNormal, sandMetal,
                sandRough);
        for (const auto& c : corals)
            drawPBR(pbrShader, c.mesh, c.transform, c.albedoTint, c.metallic, c.roughness, coralAlbedo, coralNormal,
                    coralMetal, coralRough);
        for (const auto& r : rocks)
            drawPBR(pbrShader, r.mesh, r.transform, r.albedoTint, r.metallic, r.roughness, sandAlbedo, sandNormal,
                    sandMetal, sandRough);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
