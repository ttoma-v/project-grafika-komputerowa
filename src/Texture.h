#pragma once

#include <string>
#include <vector>

class Texture2D {
public:
    unsigned int id = 0;
    int width = 0;
    int height = 0;

    void createRGBA(int w, int h, const std::vector<unsigned char>& pixels, bool mipmaps = true);
    static Texture2D loadFromFile(const std::string& path, bool mipmaps = true);
    static Texture2D loadFromMemory(const unsigned char* data, int size, bool mipmaps = true);
    void bind(unsigned int unit) const;
    void destroy();
};

class TextureCube {
public:
    unsigned int id = 0;

    void createFromFaces(int size, const std::vector<std::vector<unsigned char>>& facesRGBA);
    void bind(unsigned int unit) const;
    void destroy();
};

namespace ProceduralTextures {
Texture2D makeSandAlbedo(int size = 256);
Texture2D makeSandNormal(int size = 256);
Texture2D makeCoralAlbedo(int size = 128);
Texture2D makeCoralNormal(int size = 128);
Texture2D makeKelpAlbedo(int size = 64);
Texture2D makeFlatNormal(int size = 4);
Texture2D makeRoughness(int size = 64, float base = 0.75f);
Texture2D makeMetallic(int size = 64, float base = 0.05f);
Texture2D makeFlowMap(int size = 256);
TextureCube makeUnderwaterSky(int size = 256);
}
