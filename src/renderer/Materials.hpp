#pragma once

#include "renderer/TextureAtlas.hpp"

struct Material {
    TextureId albedo{TextureId::Concrete};
    float ambient{1.0f};
    float emissive{0.0f};
    bool transparent{false};
    bool animated{false};
};

class MaterialLibrary {
  public:
    Material wall(char tile, int mapX, int mapY, float time) const;
    Material floor(int mapX, int mapY) const;
    Material ceiling(int mapX, int mapY) const;
};
