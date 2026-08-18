#pragma once

#include <cstdint>
#include <string>
#include <vector>

class SpriteSheet {
public:
  bool load(const std::string& path, int columns);
  bool valid() const { return !pixels_.empty(); }
  void draw(std::vector<std::uint32_t>& target, int targetWidth, int targetHeight,
            int frame, int x, int y, int width, int height, float brightness = 1.0f) const;

private:
  int width_{}, height_{}, columns_{1};
  std::vector<std::uint32_t> pixels_;
};
