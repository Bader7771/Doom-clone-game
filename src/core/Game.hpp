#pragma once
#include "audio/Audio.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "input/Input.hpp"
#include "renderer/Renderer.hpp"
#include "ui/HudFace.hpp"
#include "weapons/Shotgun.hpp"
#include "world/Level.hpp"
#include <SDL3/SDL.h>
#include <vector>
enum class GameState { Title, Transitioning, Loading, Playing };

class Game {
  public:
    bool init();
    int run();
    ~Game();

  private:
    void update(float dt);
    SDL_Window* window_{};
    Input input_;
    Renderer renderer_;
    Level level_;
    Player player_;
    Shotgun shotgun_;
    HudFace hudFace_;
    Audio audio_;
    std::vector<Enemy> enemies_;
    std::vector<EnemyProjectile> enemyProjectiles_;
    float deathTimer_{};
    bool won_{}, fullscreen_{}, debugEnemies_{};
    
    GameState state_{GameState::Title};
    float transitionTimer_{0.0f};
    int menuSelection_{0};
};
