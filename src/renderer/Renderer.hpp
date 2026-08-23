#pragma once

#include "renderer/Materials.hpp"
#include "renderer/SpriteSheet.hpp"
#include "renderer/TextureAtlas.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <cstring>
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
    void draw(const Level&,
              const Player&,
              const std::vector<Enemy>&,
              const std::vector<EnemyProjectile>&,
              const Shotgun&,
              const HudFace&,
              bool won,
              bool debugEnemies = false);
    void drawTitleScreen(int selection, float dt);
    void drawTransition(float progress);
    void drawLoadingScreen(float dt);
    void shutdown();

    // Pickup notification: call from Game when a pickup is collected
    void pushNotification(const char* text, std::uint32_t color = 0xfff1e8c9u);

  private:
    void pixel(int x, int y, std::uint32_t color);
    void rect(int x, int y, int w, int h, std::uint32_t color);
    void text(int x, int y, const char* txt, std::uint32_t color, int scale = 1);
    void sprite(
        int centerX, int baseY, int size, std::uint32_t body, std::uint32_t eye, float animation);
    void worldSprite(float x,
                     float y,
                     int kind,
                     const Player&,
                     const std::vector<float>& depth,
                     float animation = 0.0f,
                     float pain = 0.0f);
    void enemySprite(const Enemy&, const Player&, bool debug);
    void drawSurfaces(const Level&, const Player&, const Shotgun&);
    void drawNotifications();
    void present();

    // --------------- Pickup notification queue ---------------
    struct PickupNote {
        char text[64];
        float timer;
        float maxTimer;
        std::uint32_t color;
    };
    static constexpr int MaxNotes = 4;
    PickupNote notes_[MaxNotes]{};
    int noteCount_{};
    
    struct TitleParticle {
        float x, y;
        float vx, vy;
        float life, maxLife;
        int type; // 0=steam, 1=spark
    };
    std::vector<TitleParticle> titleParticles_;
    float nextSparkTime_{};

    SDL_Window* window_{};
    SDL_GLContext context_{};
    unsigned texture_{};
    float time_{};
    TextureAtlas atlas_;
    MaterialLibrary materials_;
    SpriteSheet shotgunSprites_;
    SpriteSheet faceSprites_;
    SpriteSheet pickupSprites_; // 4-column sheet: health, ammo, key, battery
    SpriteSheet titleBgSprite_;
    std::array<SpriteSheet, 4> enemySprites_;
    std::vector<std::uint32_t> pixels_ = std::vector<std::uint32_t>(W * H);
    std::vector<float> depth_ = std::vector<float>(W, 99.0f);
};
