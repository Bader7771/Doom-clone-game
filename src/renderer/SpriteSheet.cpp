#include "renderer/SpriteSheet.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

bool SpriteSheet::load(const std::string& path, int columns) {
  SDL_Surface* loaded = SDL_LoadPNG(path.c_str());
  if (!loaded) return false;
  SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);
  SDL_DestroySurface(loaded);
  if (!surface) return false;
  width_ = surface->w;
  height_ = surface->h;
  columns_ = std::max(1, columns);
  pixels_.resize(static_cast<std::size_t>(width_) * height_);
  const auto* source = static_cast<const std::uint32_t*>(surface->pixels);
  const int pitch = surface->pitch / static_cast<int>(sizeof(std::uint32_t));
  for (int y = 0; y < height_; ++y)
    std::copy_n(source + y * pitch, width_, pixels_.begin() + static_cast<std::size_t>(y) * width_);
  SDL_DestroySurface(surface);
  return true;
}

void SpriteSheet::draw(std::vector<std::uint32_t>& target, int targetWidth, int targetHeight,
                       int frame, int x, int y, int width, int height, float brightness) const {
  if (!valid() || width <= 0 || height <= 0) return;
  const int cellWidth = width_ / columns_;
  frame = std::clamp(frame, 0, columns_ - 1);
  for (int dy = 0; dy < height; ++dy) {
    const int ty = y + dy;
    if (ty < 0 || ty >= targetHeight) continue;
    const int sy = dy * height_ / height;
    for (int dx = 0; dx < width; ++dx) {
      const int tx = x + dx;
      if (tx < 0 || tx >= targetWidth) continue;
      const int sx = frame * cellWidth + dx * cellWidth / width;
      const std::uint32_t source = pixels_[static_cast<std::size_t>(sy) * width_ + sx];
      const unsigned alpha = source >> 24u;
      if (alpha < 24u) continue;
      const unsigned r = std::min(255u, static_cast<unsigned>(((source >> 16u) & 255u) * brightness));
      const unsigned g = std::min(255u, static_cast<unsigned>(((source >> 8u) & 255u) * brightness));
      const unsigned b = std::min(255u, static_cast<unsigned>((source & 255u) * brightness));
      target[static_cast<std::size_t>(ty) * targetWidth + tx] = 0xff000000u | (r << 16u) | (g << 8u) | b;
    }
  }
}
