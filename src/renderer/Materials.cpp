#include "renderer/Materials.hpp"

#include <cmath>

Material MaterialLibrary::wall(char tile, int x, int y, float time) const {
  if (tile == 'D') return {TextureId::SecurityDoor, 0.9f, 0.35f};
  const unsigned hash = static_cast<unsigned>(x * 37 + y * 71);
  if ((x == 5 || x == 14) && y % 4 == 2) return {TextureId::WarningPanel, 0.88f};
  if ((y == 1 || y == 6) && x % 7 == 0) {
    const float pulse = 0.35f + 0.15f * std::sin(time * 3.0f + static_cast<float>(x));
    return {TextureId::ComputerPanel, 0.9f, pulse, false, true};
  }
  if (hash % 11 == 0) return {TextureId::RustedMetal, 0.8f};
  if (hash % 7 == 0) return {TextureId::DamagedConcrete, 0.82f};
  return hash % 3 == 0 ? Material{TextureId::IndustrialMetal, 0.9f}
                       : Material{TextureId::Concrete, 0.86f};
}

Material MaterialLibrary::floor(int x, int y) const {
  if (x >= 15) return {TextureId::HazardFloor, 0.72f};
  if (y >= 8) return {TextureId::DirtyFloor, 0.7f};
  return {TextureId::FloorPlate, 0.76f};
}

Material MaterialLibrary::ceiling(int x, int y) const {
  if ((x / 4 + y / 3) % 3 == 0) return {TextureId::CeilingPipes, 0.58f};
  return {TextureId::CeilingPanel, 0.62f};
}
