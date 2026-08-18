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
  void drawCell(std::vector<std::uint32_t>& target, int targetWidth, int targetHeight,
                int column, int row, int rows, int x, int y, int width, int height,
                float brightness = 1.0f, bool trimGutters = true) const;

private:
  int width_{}, height_{}, columns_{1};
  std::vector<std::uint32_t> pixels_;
};
