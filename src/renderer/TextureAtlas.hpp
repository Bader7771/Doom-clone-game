#pragma once

#include <cstdint>
#include <vector>

enum class TextureId : std::uint8_t {
  Concrete,
  DamagedConcrete,
  IndustrialMetal,
  RustedMetal,
  SecurityDoor,
  WarningPanel,
  ComputerPanel,
  FloorPlate,
  DirtyFloor,
  HazardFloor,
  CeilingPanel,
  CeilingPipes,
  Count
};

class TextureAtlas {
public:
  static constexpr int TileSize = 32;

  TextureAtlas();
  std::uint32_t sample(TextureId texture, float u, float v) const;

private:
  void build();
  void set(TextureId texture, int x, int y, std::uint32_t color);
  std::vector<std::uint32_t> pixels_;
};
