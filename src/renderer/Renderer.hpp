#pragma once

#include "renderer/Materials.hpp"
#include "renderer/TextureAtlas.hpp"
#include "renderer/SpriteSheet.hpp"
#include <SDL3/SDL.h>
#include <cstdint>
#include <array>
#include <vector>

class Level;
class Player;
class Enemy;
struct EnemyProjectile;
class Shotgun;
class HudFace;

class Renderer {
public:
  static constexpr int W = 640;
  static constexpr int H = 360;

  bool init(SDL_Window* window);
  void draw(const Level&, const Player&, const std::vector<Enemy>&, const std::vector<EnemyProjectile>&, const Shotgun&, const HudFace&, bool won, bool debugEnemies = false);
  void shutdown();

private:
  void pixel(int x, int y, std::uint32_t color);
  void rect(int x, int y, int w, int h, std::uint32_t color);
  void text(int x, int y, const char* text, std::uint32_t color, int scale = 1);
  void sprite(int centerX, int baseY, int size, std::uint32_t body, std::uint32_t eye, float animation);
  void worldSprite(float x, float y, int kind, const Player&, const std::vector<float>& depth, float animation = 0.0f, float pain = 0.0f);
  void enemySprite(const Enemy&, const Player&, bool debug);
  void drawSurfaces(const Level&, const Player&, const Shotgun&);
  void present();

  SDL_Window* window_{};
  SDL_GLContext context_{};
  unsigned texture_{};
  float time_{};
  TextureAtlas atlas_;
  MaterialLibrary materials_;
  SpriteSheet shotgunSprites_;
  SpriteSheet faceSprites_;
  std::array<SpriteSheet,3> enemySprites_;
  std::vector<std::uint32_t> pixels_ = std::vector<std::uint32_t>(W * H);
  std::vector<float> depth_ = std::vector<float>(W, 99.0f);
};
