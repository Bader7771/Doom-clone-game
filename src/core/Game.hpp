#pragma once
#include "audio/Audio.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "input/Input.hpp"
#include "renderer/Renderer.hpp"
#include "weapons/Shotgun.hpp"
#include "world/Level.hpp"
#include <SDL3/SDL.h>
#include <vector>
class Game {
public: bool init(); int run(); ~Game();
private: void update(float dt); SDL_Window* window_{}; Input input_; Renderer renderer_; Level level_; Player player_; Shotgun shotgun_; Audio audio_; std::vector<Enemy> enemies_; bool won_{};
};

