#include "Texture.h"

#include "GlLoader.h"

#include <glm/glm.hpp>

#include <cmath>

static float hash2d(int x, int y, int seed) {
    float n = static_cast<float>(x * 374761393 + y * 668265263 + seed * 982451653);
    n = std::fmod(std::sin(n) * 43758.5453f, 1.0f);
    return n < 0.0f ? n + 1.0f : n;
}

static float smoothNoise(float x, float y, int seed) {
    const int xi = static_cast<int>(std::floor(x));
    const int yi = static_cast<int>(std::floor(y));
    const float tx = x - xi;
    const float ty = y - yi;
    const float a = hash2d(xi, yi, seed);
    const float b = hash2d(xi + 1, yi, seed);
    const float c = hash2d(xi, yi + 1, seed);
    const float d = hash2d(xi + 1, yi + 1, seed);
    const float ux = tx * tx * (3.0f - 2.0f * tx);
    const float uy = ty * ty * (3.0f - 2.0f * ty);
    return glm::mix(glm::mix(a, b, ux), glm::mix(c, d, ux), uy);
}

void Texture2D::createRGBA(int w, int h, const std::vector<unsigned char>& pixels, bool mipmaps) {
    width = w;
    height = h;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mipmaps) glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2D::destroy() {
    if (id) glDeleteTextures(1, &id);
    id = 0;
}

void TextureCube::createFromFaces(int size, const std::vector<std::vector<unsigned char>>& facesRGBA) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     facesRGBA[i].data());
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void TextureCube::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}

void TextureCube::destroy() {
    if (id) glDeleteTextures(1, &id);
    id = 0;
}

namespace ProceduralTextures {

TextureCube makeUnderwaterSky(int size) {
    std::vector<std::vector<unsigned char>> faces(6, std::vector<unsigned char>(size * size * 4));
    for (int face = 0; face < 6; ++face) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const float v = static_cast<float>(y) / static_cast<float>(size);
                const float haze = smoothNoise(x * 0.03f + face * 10.0f, y * 0.03f, 77);
                const int i = (y * size + x) * 4;
                faces[face][i + 0] = static_cast<unsigned char>(5 + v * 15 + haze * 10);
                faces[face][i + 1] = static_cast<unsigned char>(25 + v * 40 + haze * 20);
                faces[face][i + 2] = static_cast<unsigned char>(45 + v * 80 + haze * 30);
                faces[face][i + 3] = 255;
            }
        }
    }
    TextureCube cube;
    cube.createFromFaces(size, faces);
    return cube;
}

}
