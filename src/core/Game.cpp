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
        const EnemyType type = std::string_view(capture) == "GUNNER"  ? EnemyType::Gunner
                               : std::string_view(capture) == "BRUTE" ? EnemyType::Brute
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
    player_.update(dt, input_, level_);
    Vec2 f{std::cos(player_.angle), std::sin(player_.angle)};
    if (input_.interact())
        level_.tryOpenDoor(player_.pos, f, player_.hasKey);
    shotgun_.update(dt, input_.fire(), player_, level_, enemies_, audio_);
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
            } else if (q.type == PickupType::Ammo) {
                player_.ammo += 8;
                q.active = false;
            } else if (q.type == PickupType::Key) {
                player_.hasKey = true;
                q.active = false;
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
        Uint64 now = SDL_GetTicksNS();
        float dt = std::min(.05f, (now - last) / 1000000000.f);
        last = now;
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
    return 0;
}
