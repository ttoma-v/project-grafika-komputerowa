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
    Shader unlitShader;
    const std::string skyVert = assetPath("shaders/skybox.vert");
    const std::string skyFrag = assetPath("shaders/skybox.frag");
    const std::string unlitVert = assetPath("shaders/unlit.vert");
    const std::string unlitFrag = assetPath("shaders/unlit.frag");

    if (!skyShader.loadFromFiles(skyVert, skyFrag) || !unlitShader.loadFromFiles(unlitVert, unlitFrag)) {
        std::cerr << "Shader load failed. Run exe from build/Release or ensure assets/ is nearby.\n";
        std::cerr << "Tried: " << skyVert << '\n';
        return 1;
    }
    std::cout << "Shaders OK. Assets from: " << fs::current_path() << '\n';

    auto skyCube = ProceduralTextures::makeUnderwaterSky();
    Mesh skyboxMesh = makeSkyboxCube();
    const float kSeabedSize = 80.0f;
    Mesh seabed = Geometry::makePlane(kSeabedSize, kSeabedSize, 32);
    const glm::mat4 floorTransform(1.0f);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        const float time = static_cast<float>(glfwGetTime());
        const float dt = time - lastFrame;
        lastFrame = time;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        gCamera.processKeyboard(gKeys[GLFW_KEY_W], gKeys[GLFW_KEY_S], gKeys[GLFW_KEY_A], gKeys[GLFW_KEY_D],
                                gKeys[GLFW_KEY_SPACE], gKeys[GLFW_KEY_LEFT_SHIFT], dt);

        const glm::mat4 proj = gCamera.projectionMatrix(static_cast<float>(gWidth) / static_cast<float>(gHeight));
        const glm::mat4 view = gCamera.viewMatrix();

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

        unlitShader.use();
        unlitShader.setMat4("view", view);
        unlitShader.setMat4("projection", proj);
        unlitShader.setMat4("model", floorTransform);
        unlitShader.setVec3("color", glm::vec3(0.55f, 0.48f, 0.35f));
        seabed.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
