#pragma once

#include <vector>

class Texture2D {
public:
    unsigned int id = 0;
    int width = 0;
    int height = 0;

    void createRGBA(int w, int h, const std::vector<unsigned char>& pixels, bool mipmaps = true);
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
TextureCube makeUnderwaterSky(int size = 256);
}
