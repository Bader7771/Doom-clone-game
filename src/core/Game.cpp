#include "core/Game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string_view>

bool Game::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        return false;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    window_ =
        SDL_CreateWindow("VOIDLOCK",
                         1280,
                         720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_)
        return false;
    if (!renderer_.init(window_))
        return false;
    SDL_SetWindowRelativeMouseMode(window_, true);
    audio_.init();
    for (const auto& spawn : level_.enemySpawns)
        enemies_.emplace_back(spawn.type, spawn.pos);
    if (const char* capture = std::getenv("VOIDLOCK_CAPTURE_ENEMY")) {
        enemies_.clear();
        const EnemyType type = std::string_view(capture) == "GUNNER"   ? EnemyType::Gunner
                               : std::string_view(capture) == "BRUTE"  ? EnemyType::Brute
                                                                       : EnemyType::Rusher;
        enemies_.emplace_back(type, Vec2{4.15f, 2.5f});
    }
    if (const char* face = std::getenv("VOIDLOCK_CAPTURE_FACE")) {
        const std::string_view state(face);
        if (state == "CRITICAL")
            player_.health = 20;
        else if (state == "PAIN_LEFT") {
            player_.health = 65;
            player_.hurt(18, {player_.pos.x, player_.pos.y - 1.f});
        } else if (state == "DEAD")
            player_.hurt(100, {player_.pos.x + 1.f, player_.pos.y});
        else if (state == "KILL" && !enemies_.empty())
            enemies_.front().damage(999);
    }
    return true;
}
Game::~Game() {
    renderer_.shutdown();
    if (window_)
        SDL_DestroyWindow(window_);
    SDL_Quit();
}
void Game::update(float dt) {
    if (won_ || player_.health <= 0) {
        hudFace_.update(dt, player_, shotgun_, enemies_);
        return;
    }
    // --- Flashlight: battery drain ---
    if (player_.flashlightOn) {
        player_.flashlightCharge -= 2.0f * dt;
        if (player_.flashlightCharge <= 0.f) {
            player_.flashlightCharge = 0.f;
            player_.flashlightOn = false;
        }
    }
    player_.update(dt, input_, level_);
    Vec2 f{std::cos(player_.angle), std::sin(player_.angle)};
    if (input_.interact())
        level_.tryOpenDoor(player_.pos, f, player_.hasKey);
    const bool justFired = input_.fire() && player_.ammo > 0 && player_.health > 0;
    shotgun_.update(dt, input_.fire(), player_, level_, enemies_, audio_);
    // --- Shotgun noise: alert idle zombies within range ---
    if (justFired) {
        constexpr float NoiseRange = 18.0f;
        for (auto& e : enemies_)
            if (e.state == EnemyState::Idle && length(e.pos - player_.pos) < NoiseRange)
                e.state = EnemyState::Alert;
    }
    for (auto& e : enemies_)
        e.update(dt, player_, level_, enemyProjectiles_, audio_);
    for (auto& projectile : enemyProjectiles_) {
        projectile.life -= dt;
        if (!projectile.exploding) {
            projectile.pos += projectile.velocity * dt;
            if (level_.solid(projectile.pos.x, projectile.pos.y) ||
                length(projectile.pos - player_.pos) < .32f) {
                projectile.exploding = true;
                projectile.explosionTime = 0;
            }
        } else {
            projectile.explosionTime += dt;
            if (!projectile.damageApplied) {
                projectile.damageApplied = true;
                const float distance = length(projectile.pos - player_.pos);
                if (distance < 1.55f) {
                    player_.hurt(static_cast<int>(projectile.damage * (1.f - distance / 1.55f)),
                                 projectile.pos - projectile.velocity * .2f);
                    player_.addWeaponKick(.025f, .65f);
                }
            }
        }
    }
    std::erase_if(enemyProjectiles_, [](const EnemyProjectile& projectile) {
        return projectile.life <= 0 || (projectile.exploding && projectile.explosionTime > .42f);
    });
    for (auto& q : level_.pickups)
        if (q.active && length(q.pos - player_.pos) < .5f) {
            if (q.type == PickupType::Health && player_.health < 100) {
                player_.health = std::min(100, player_.health + 35);
                q.active = false;
                renderer_.pushNotification("+35 HEALTH", 0xff47c95eu);
            } else if (q.type == PickupType::Ammo) {
                player_.ammo += 8;
                q.active = false;
                renderer_.pushNotification("+8 SHELLS", 0xffd6b744u);
            } else if (q.type == PickupType::Key) {
                player_.hasKey = true;
                q.active = false;
                renderer_.pushNotification("KEYCARD FOUND", 0xff45cfe8u);
            } else if (q.type == PickupType::FlashlightBattery) {
                player_.flashlightCharge =
                    std::min(player_.flashlightCharge + 35.0f, Player::MaxFlashlightCharge);
                q.active = false;
                renderer_.pushNotification("BATTERY +35%", 0xffb044d0u);
            }
            if (!q.active)
                audio_.playPickup();
        }
    bool finalRoomClear = true;
    for (const auto& e : enemies_)
        if (e.state != EnemyState::Dead && e.pos.x > 15.f)
            finalRoomClear = false;
    if (level_.atExit(player_.pos) && finalRoomClear)
        won_ = true;
    hudFace_.update(dt, player_, shotgun_, enemies_);
}
int Game::run() {
    Uint64 last = SDL_GetTicksNS();
    bool captureShot = std::getenv("VOIDLOCK_CAPTURE_FIRE") != nullptr;
    while (input_.update()) {
        if (input_.fullscreenToggle()) {
            fullscreen_ = !fullscreen_;
            SDL_SetWindowFullscreen(window_, fullscreen_);
        }
        if (input_.pressed(SDL_SCANCODE_F1))
            debugEnemies_ = !debugEnemies_;
        // --- Flashlight toggle ---
        if (input_.flashlightToggle()) {
            if (!player_.flashlightOn && player_.flashlightCharge > 0.f) {
                player_.flashlightOn = true;
                char msg[32];
                std::snprintf(msg,
                              sizeof(msg),
                              "FL:ON  [%d%%]",
                              static_cast<int>(player_.flashlightCharge /
                                               Player::MaxFlashlightCharge * 100.f));
                renderer_.pushNotification(msg, 0xff53f6d0u);
            } else {
                player_.flashlightOn = false;
                renderer_.pushNotification("FL:OFF", 0xff888888u);
            }
        }
        Uint64 now = SDL_GetTicksNS();
        float dt = std::min(.05f, (now - last) / 1000000000.f);
        last = now;

        if (state_ == GameState::Title) {
            if (input_.pressed(SDL_SCANCODE_W) || input_.pressed(SDL_SCANCODE_UP)) {
                menuSelection_ = std::max(0, menuSelection_ - 1);
            }
            if (input_.pressed(SDL_SCANCODE_S) || input_.pressed(SDL_SCANCODE_DOWN)) {
                menuSelection_ = std::min(3, menuSelection_ + 1);
            }
            if (input_.pressed(SDL_SCANCODE_RETURN) || input_.pressed(SDL_SCANCODE_SPACE)) {
                if (menuSelection_ == 0 || menuSelection_ == 1) {
                    state_ = GameState::Transitioning;
                    transitionTimer_ = 1.0f;
                } else if (menuSelection_ == 3) {
                    break; // EXIT
                }
            }
            renderer_.drawTitleScreen(menuSelection_, dt);
        } else if (state_ == GameState::Transitioning) {
            transitionTimer_ -= dt;
            if (transitionTimer_ <= 0.0f) {
                state_ = GameState::Loading;
                transitionTimer_ = 1.5f; // Short loading screen
            }
            renderer_.drawTransition(1.0f - transitionTimer_);
        } else if (state_ == GameState::Loading) {
            transitionTimer_ -= dt;
            if (transitionTimer_ <= 0.0f) {
                state_ = GameState::Playing;
            }
            renderer_.drawLoadingScreen(dt);
        } else if (state_ == GameState::Playing) {
            if (captureShot) {
                shotgun_.update(0, true, player_, level_, enemies_, audio_);
                captureShot = false;
            }
            update(dt);
            renderer_.draw(
                level_, player_, enemies_, enemyProjectiles_, shotgun_, hudFace_, won_, debugEnemies_);
            if (player_.health <= 0) {
                deathTimer_ += dt;
                if (deathTimer_ > 1.2f) {
                    player_.health = 100;
                    player_.pos = {2.5f, 2.5f};
                    deathTimer_ = 0;
                }
            } else
                deathTimer_ = 0;
        }
    }
    return 0;
}
