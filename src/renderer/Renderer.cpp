#include "renderer/Renderer.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "ui/HudFace.hpp"
#include "weapons/Shotgun.hpp"
#include "world/Level.hpp"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr float PI = 3.14159265f;
constexpr float FOV = PI / 3.0f;

// ---------------------------------------------------------------------------
// Flashlight: returns a [0,1] intensity for a point at world position (wx,wy)
// relative to the player's position and facing. The cone is perspective-correct
// (accounts for vertical offset via a virtual z component).
// ---------------------------------------------------------------------------
struct FlashlightCtx {
    bool on;
    float flicker; // 0..1 from Player::getFlashlightFlicker
    Vec2 playerPos;
    float playerAngle;
};

float flashlightFactor(const FlashlightCtx& fl, float wx, float wy, float dz = 0.f) {
    if (!fl.on || fl.flicker <= 0.f)
        return 0.f;
    const float dx = wx - fl.playerPos.x;
    const float dy = wy - fl.playerPos.y;
    const float dist2d = std::sqrt(dx * dx + dy * dy);
    if (dist2d < 0.05f)
        return fl.flicker * 2.0f;
    // Forward dot-product: how much is the point in front of the player
    const float fwdX = std::cos(fl.playerAngle);
    const float fwdY = std::sin(fl.playerAngle);
    const float cosTheta = (dx * fwdX + dy * fwdY) / dist2d; // angle to point on floor
    // Account for vertical viewing angle using the ray z component
    const float dist3d = std::sqrt(dist2d * dist2d + dz * dz);
    const float cosTheta3d = dist2d / dist3d; // cosine of the vertical elevation
    const float combined = cosTheta * cosTheta3d;
    // Cone: full bright at ~15 deg (cos~0.97), falloff to 35 deg (cos~0.82)
    const float coneMin = 0.80f, coneMax = 0.96f;
    const float coneFactor = std::clamp((combined - coneMin) / (coneMax - coneMin), 0.f, 1.f);
    // Distance falloff: bright to ~5 units, dark at 11
    const float distFactor = std::clamp(1.f - dist3d / 11.f, 0.f, 1.f);
    return fl.flicker * coneFactor * coneFactor * distFactor * 2.2f;
}

std::uint32_t shade(std::uint32_t color, float amount) {
    amount = std::clamp(amount, 0.0f, 1.5f);
    const auto r =
        static_cast<unsigned>(std::min(255.0f, static_cast<float>((color >> 16u) & 255u) * amount));
    const auto g =
        static_cast<unsigned>(std::min(255.0f, static_cast<float>((color >> 8u) & 255u) * amount));
    const auto b =
        static_cast<unsigned>(std::min(255.0f, static_cast<float>(color & 255u) * amount));
    return 0xff000000u | (r << 16u) | (g << 8u) | b;
}
} // namespace

bool Renderer::init(SDL_Window* window) {
    window_ = window;
    context_ = SDL_GL_CreateContext(window);
    if (!context_)
        return false;
    SDL_GL_SetSwapInterval(1);
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels_.data());
    if (!shotgunSprites_.load("assets/sprites/weapons/rivet12_sheet_source.png", 6) &&
        !shotgunSprites_.load("../assets/sprites/weapons/rivet12_sheet_source.png", 6)) {
        SDL_Log("Weapon sprite load failed: %s", SDL_GetError());
    }
    if (!enemySprites_[0].load("assets/sprites/enemies/rusher_atlas.png", 8) &&
        !enemySprites_[0].load("../assets/sprites/enemies/rusher_atlas.png", 8))
        SDL_Log("Rusher sprite load failed: %s", SDL_GetError());
    if (!enemySprites_[1].load("assets/sprites/enemies/gunner_atlas.png", 8) &&
        !enemySprites_[1].load("../assets/sprites/enemies/gunner_atlas.png", 8))
        SDL_Log("Gunner sprite load failed: %s", SDL_GetError());
    if (!enemySprites_[2].load("assets/sprites/enemies/brute_atlas.png", 8) &&
        !enemySprites_[2].load("../assets/sprites/enemies/brute_atlas.png", 8))
        SDL_Log("Brute sprite load failed: %s", SDL_GetError());
    if (!enemySprites_[3].load("assets/sprites/enemies/zombie_atlas.png", 8) &&
        !enemySprites_[3].load("../assets/sprites/enemies/zombie_atlas.png", 8) &&
        !enemySprites_[3].load("assets/sprites/enemies/rusher_atlas.png", 8) &&
        !enemySprites_[3].load("../assets/sprites/enemies/rusher_atlas.png", 8))
        SDL_Log("Zombie sprite load failed: %s", SDL_GetError());
    // 4-column pickup sheet: 0=health, 1=ammo, 2=keycard, 3=battery
    if (!pickupSprites_.load("assets/sprites/pickups_sheet.png", 4) &&
        !pickupSprites_.load("../assets/sprites/pickups_sheet.png", 4))
        SDL_Log("Pickup sprite load failed: %s", SDL_GetError());
    if (!titleBgSprite_.load("assets/sprites/ui/title_bg.png", 1) &&
        !titleBgSprite_.load("../assets/sprites/ui/title_bg.png", 1))
        SDL_Log("Title BG load failed: %s", SDL_GetError());
    if (!faceSprites_.load("assets/sprites/ui/status_face_atlas.png", 10) &&
        !faceSprites_.load("../assets/sprites/ui/status_face_atlas.png", 10))
        SDL_Log("HUD face sprite load failed: %s", SDL_GetError());
    return true;
}

void Renderer::shutdown() {
    if (texture_)
        glDeleteTextures(1, &texture_);
    if (context_)
        SDL_GL_DestroyContext(context_);
    texture_ = 0;
    context_ = nullptr;
}

void Renderer::pixel(int x, int y, std::uint32_t color) {
    if (x >= 0 && y >= 0 && x < W && y < H)
        pixels_[y * W + x] = color;
}

void Renderer::rect(int x, int y, int w, int h, std::uint32_t color) {
    for (int yy = std::max(0, y); yy < std::min(H, y + h); ++yy)
        for (int xx = std::max(0, x); xx < std::min(W, x + w); ++xx)
            pixels_[yy * W + xx] = color;
}

void Renderer::text(int x, int y, const char* value, std::uint32_t color, int scale) {
    static const char* glyphs = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-";
    static const unsigned rows[] = {
        0,       0x1e9f9, 0x1e97a, 0x0f842, 0x1e8f9, 0x1f87c, 0x1f870, 0x0f85b, 0x118f1, 0x1f210,
        0x0214a, 0x11971, 0x1087c, 0x11dd1, 0x11bd1, 0x0e8ae, 0x1e8f0, 0x0e8b5, 0x1e8f1, 0x0f07a,
        0x1f210, 0x118ae, 0x118a4, 0x11d77, 0x11551, 0x11544, 0x1f124, 0x0eaae, 0x04644, 0x1e1e1,
        0x1e1e1, 0x12aaa, 0x1f1e1, 0x1f03e, 0x1087f, 0x1f0f1, 0x1f0ff, 0x1f111, 0x00040, 0x00400};
    for (; *value; ++value, x += 6 * scale) {
        const char* found = std::strchr(glyphs, *value);
        const unsigned bits = found ? rows[found - glyphs] : 0;
        for (int yy = 0; yy < 5; ++yy)
            for (int xx = 0; xx < 5; ++xx)
                if (bits & (1u << (yy * 5 + 4 - xx)))
                    rect(x + xx * scale, y + yy * scale, scale, scale, color);
    }
}

void Renderer::drawSurfaces(const Level&, const Player& player, const Shotgun& gun) {
    const float visualAngle = player.angle + std::sin(time_ * 73.f) * player.screenShake() * .004f;
    const Vec2 direction{std::cos(visualAngle), std::sin(visualAngle)};
    const float planeLength = std::tan(FOV * 0.5f);
    const Vec2 plane{-direction.y * planeLength, direction.x * planeLength};
    const Vec2 rayLeft = direction - plane;
    const Vec2 rayRight = direction + plane;
    const int horizon = H / 2;

    // Horror: very dark ceiling (near-black)
    std::fill(pixels_.begin(), pixels_.begin() + horizon * W, 0xff050609u);

    const FlashlightCtx fl{
        player.flashlightOn, player.getFlashlightFlicker(time_), player.pos, player.angle};

    for (int y = horizon + 1; y < H; ++y) {
        const float rowDistance = (0.5f * H) / static_cast<float>(y - horizon);
        Vec2 floorPos = player.pos + rayLeft * rowDistance;
        const Vec2 step = (rayRight - rayLeft) * (rowDistance / W);
        // Horror: very low ambient fog; flashlight provides most light
        const float fog = std::clamp(0.06f - rowDistance / 120.0f, 0.02f, 0.08f);
        const float muzzleLight =
            gun.flashStrength() * std::clamp(1.f - rowDistance / 6.f, 0.f, 1.f) * .85f;
        // vertical z offset (in world-units) for flashlight cone calculation
        const float dz = rowDistance * 0.5f; // floor is below eye level
        for (int x = 0; x < W; ++x) {
            const int mapX = static_cast<int>(std::floor(floorPos.x));
            const int mapY = static_cast<int>(std::floor(floorPos.y));
            const float u = floorPos.x - std::floor(floorPos.x);
            const float v = floorPos.y - std::floor(floorPos.y);
            const Material floorMat = materials_.floor(mapX, mapY);
            const Material ceilMat = materials_.ceiling(mapX, mapY);
            const float fl_floor = flashlightFactor(fl, floorPos.x, floorPos.y, dz);
            const float fl_ceil = flashlightFactor(fl, floorPos.x, floorPos.y, -dz);
            pixel(x,
                  y,
                  shade(atlas_.sample(floorMat.albedo, u, v),
                        fog * floorMat.ambient + fl_floor + muzzleLight));
            pixel(x,
                  H - y,
                  shade(atlas_.sample(ceilMat.albedo, u, v),
                        fog * ceilMat.ambient + fl_ceil * 0.6f + muzzleLight * .8f));
            floorPos += step;
        }
    }
}

void Renderer::sprite(
    int centerX, int baseY, int size, std::uint32_t body, std::uint32_t eye, float animation) {
    const int top = baseY - size;
    for (int y = 0; y < size; ++y) {
        for (int x = -size / 2; x < size / 2; ++x) {
            const float nx = x / (size * 0.5f);
            const float ny = (y - size * 0.48f) / (size * 0.52f);
            if (nx * nx + ny * ny < 1.0f)
                pixel(centerX + x, top + y, shade(body, 0.72f + 0.28f * (1.0f - std::abs(nx))));
        }
    }
    const int bob = static_cast<int>(std::sin(animation * 7.0f) * size * 0.03f);
    rect(centerX - size / 5, top + size / 3 + bob, size / 9, size / 9, eye);
    rect(centerX + size / 10, top + size / 3 + bob, size / 9, size / 9, eye);
    rect(centerX - size / 3, top + size * 3 / 4, size * 2 / 3, std::max(1, size / 12), 0xff171523u);
}

void Renderer::worldSprite(float x,
                           float y,
                           int kind,
                           const Player& player,
                           const std::vector<float>& depth,
                           float /*animation*/,
                           float pain) {
    const float dx = x - player.pos.x;
    const float dy = y - player.pos.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float angle = wrapAngle(std::atan2(dy, dx) - player.angle);
    if (std::abs(angle) > FOV * 0.7f || distance < 0.15f)
        return;
    const int screenX =
        static_cast<int>(W * 0.5f + std::tan(angle) / std::tan(FOV * 0.5f) * W * 0.5f);
    const int size = static_cast<int>(H / (distance * 0.9f));
    // Subtle bob animation for pickups
    const float bob = std::sin(time_ * 2.8f + x * 1.3f + y * 0.7f) * std::max(1.f, size * 0.06f);
    const int base = H / 2 + size / 2 + static_cast<int>(bob);
    if (screenX < 0 || screenX >= W || distance >= depth[std::clamp(screenX, 0, W - 1)])
        return;
    // Flashlight factor for this pickup
    const FlashlightCtx fl{
        player.flashlightOn, player.getFlashlightFlicker(time_), player.pos, player.angle};
    const float flFactor = std::clamp(flashlightFactor(fl, x, y), 0.f, 1.5f);
    // Pickups have a tiny self-glow so they're barely discoverable in the dark
    const float selfGlow = (kind == 3) ? 0.18f : 0.08f; // keycard glows more
    const float brightness = selfGlow + flFactor;

    if (kind == 0) {
        // Fallback procedural sprite (unused in normal gameplay)
        sprite(screenX, base, size, pain > 0.f ? 0xffd9bec7u : 0xff674d8fu, 0xff53f6d0u, time_);
        return;
    }

    // --- Real pickup sprites ---
    // kind: 1=health, 2=ammo, 3=key, 4=battery
    // pickupSprites_ column: 0=health, 1=ammo, 2=key, 3=battery
    const int col = kind - 1; // offset: kind 1 -> col 0, etc.
    const int spriteSize = std::max(6, size * 3 / 4);
    const int sx = screenX - spriteSize / 2;
    const int sy = base - spriteSize;

    if (pickupSprites_.valid()) {
        pickupSprites_.drawCell(
            pixels_, W, H, col, 0, 1, sx, sy, spriteSize, spriteSize, brightness);
    } else {
        // Fallback colored rectangles if sprite sheet didn't load
        const std::uint32_t color = kind == 1   ? 0xff47c95eu
                                    : kind == 2 ? 0xffd6b744u
                                    : kind == 3 ? 0xff45cfe8u
                                                : 0xffb044d0u;
        const int h = std::max(3, size / 2);
        rect(screenX - h / 3, base - h, h * 2 / 3, h, shade(color, brightness));
        rect(screenX - h / 2, base - h / 2, h, h / 3, shade(color, brightness * 0.6f));
    }
}

void Renderer::enemySprite(const Enemy& enemy, const Player& player, bool debug) {
    const Vec2 delta = enemy.pos - player.pos;
    const float distance = length(delta);
    const float angle = wrapAngle(std::atan2(delta.y, delta.x) - player.angle);
    if (std::abs(angle) > FOV * .7f || distance < .15f)
        return;
    const int screenX = static_cast<int>(W * .5f + std::tan(angle) / std::tan(FOV * .5f) * W * .5f);
    if (screenX < 0 || screenX >= W || distance >= depth_[screenX] + .2f)
        return;
    const int size = static_cast<int>(H / (distance * .9f) * enemy.definition().visualScale);
    const int base = H / 2 + size / 2;
    const int shadowW = std::max(3, size * 2 / 5), shadowH = std::max(2, size / 14);
    for (int y = 0; y < shadowH; ++y) {
        const int inset = y * shadowW / (shadowH * 3);
        rect(screenX - shadowW / 2 + inset,
             base - shadowH / 2 + y,
             shadowW - inset * 2,
             1,
             0xff141419u);
    }
    const int row = (enemy.state == EnemyState::Dying || enemy.state == EnemyState::Dead)
                        ? 6
                        : enemy.animationFrame();
    const int column = enemy.directionFrame(player);
    // Flashlight illumination on enemy sprite
    const FlashlightCtx fl{
        player.flashlightOn, player.getFlashlightFlicker(time_), player.pos, player.angle};
    const float flFactor = std::clamp(flashlightFactor(fl, enemy.pos.x, enemy.pos.y), 0.f, 1.4f);
    const float baseAmbient = 0.08f;
    float brightness = baseAmbient + flFactor;
    if (enemy.painFlash > 0)
        brightness = std::max(brightness, 1.45f);
    brightness += enemy.muzzleFlash * .9f;
    // Zombie uses sprite index 3; legacy enemies use their original indices (0,1,2)
    const std::size_t spriteIdx = static_cast<std::size_t>(enemy.type);
    enemySprites_[spriteIdx].drawCell(
        pixels_, W, H, column, row, 7, screenX - size / 2, base - size, size, size, brightness);
    if (enemy.muzzleFlash > 0 && enemy.type != EnemyType::Rusher) {
        const int flashSize = std::max(3, size / 8);
        rect(screenX + size / 5, base - size * 3 / 5, flashSize, flashSize, 0xffffe476u);
        rect(screenX + size / 5 + 2,
             base - size * 3 / 5 + 2,
             flashSize / 2,
             flashSize / 2,
             0xffffffffu);
    }
    if (debug) {
        char info[96];
        std::snprintf(info,
                      sizeof(info),
                      "%s %s H:%d D:%.1f F:%d",
                      enemy.definition().name,
                      enemy.stateName(),
                      enemy.health,
                      distance,
                      enemy.animationFrame());
        text(std::max(2, screenX - static_cast<int>(std::strlen(info)) * 3),
             std::max(2, base - size - 9),
             info,
             0xff53f6d0u);
    }
}

void Renderer::draw(const Level& level,
                    const Player& player,
                    const std::vector<Enemy>& enemies,
                    const std::vector<EnemyProjectile>& projectiles,
                    const Shotgun& gun,
                    const HudFace& hudFace,
                    bool won,
                    bool debugEnemies) {
    time_ = static_cast<float>(SDL_GetTicksNS() / 1000000000.0);
    drawSurfaces(level, player, gun);
    const float visualAngle = player.angle + std::sin(time_ * 73.f) * player.screenShake() * .004f;
    const Vec2 direction{std::cos(visualAngle), std::sin(visualAngle)};
    const float planeLength = std::tan(FOV * 0.5f);
    const Vec2 plane{-direction.y * planeLength, direction.x * planeLength};

    for (int x = 0; x < W; ++x) {
        const float cameraX = 2.0f * x / W - 1.0f;
        const Vec2 ray{direction.x + plane.x * cameraX, direction.y + plane.y * cameraX};
        int mapX = static_cast<int>(player.pos.x);
        int mapY = static_cast<int>(player.pos.y);
        const float deltaX = ray.x == 0.0f ? 1e30f : std::abs(1.0f / ray.x);
        const float deltaY = ray.y == 0.0f ? 1e30f : std::abs(1.0f / ray.y);
        const int stepX = ray.x < 0.0f ? -1 : 1;
        const int stepY = ray.y < 0.0f ? -1 : 1;
        float sideX = (ray.x < 0.0f ? player.pos.x - mapX : mapX + 1.0f - player.pos.x) * deltaX;
        float sideY = (ray.y < 0.0f ? player.pos.y - mapY : mapY + 1.0f - player.pos.y) * deltaY;
        bool side = false;
        char hit = '#';
        for (int guard = 0; guard < 64; ++guard) {
            if (sideX < sideY) {
                sideX += deltaX;
                mapX += stepX;
                side = false;
            } else {
                sideY += deltaY;
                mapY += stepY;
                side = true;
            }
            hit = level.tile(mapX, mapY);
            if (hit == '#' || hit == 'D')
                break;
        }
        const float distance = std::max(0.001f, side ? sideY - deltaY : sideX - deltaX);
        depth_[x] = distance;
        const int wallHeight = std::min(H * 3, static_cast<int>(H / distance));
        const int top = H / 2 - wallHeight / 2;
        float wallX = side ? player.pos.x + distance * ray.x : player.pos.y + distance * ray.y;
        wallX -= std::floor(wallX);
        if ((!side && ray.x > 0.0f) || (side && ray.y < 0.0f))
            wallX = 1.0f - wallX;
        const Material material = materials_.wall(hit, mapX, mapY, time_);
        // Horror: much lower ambient, flashlight drives most wall illumination
        const float fog = std::clamp(0.06f - distance / 90.0f, 0.02f, 0.08f);
        const float muzzleLight =
            gun.flashStrength() * std::clamp(1.f - distance / 6.f, 0.f, 1.f) * .9f;
        // Hit point in world space (mid-height of wall)
        const float hitX = player.pos.x + distance * ray.x;
        const float hitY = player.pos.y + distance * ray.y;
        const FlashlightCtx fl{
            player.flashlightOn, player.getFlashlightFlicker(time_), player.pos, player.angle};
        for (int y = std::max(0, top); y < std::min(H, top + wallHeight); ++y) {
            float v = static_cast<float>(y - top) / wallHeight;
            if (material.animated)
                v += time_ * 0.12f;
            // Vertical offset in world-units for flashlight vertical falloff
            const float dz = (static_cast<float>(y - H / 2) / wallHeight) * 1.0f;
            const float flWall = flashlightFactor(fl, hitX, hitY, dz);
            const float light = fog * material.ambient * (side ? 0.78f : 1.0f) + material.emissive +
                                muzzleLight + flWall;
            pixel(x, y, shade(atlas_.sample(material.albedo, wallX, v), light));
        }
    }

    struct Visible {
        float distance;
        const Enemy* enemy;
        float x, y;
        int kind;
    };
    std::vector<Visible> sprites;
    sprites.reserve(enemies.size() + level.pickups.size());
    for (const auto& enemy : enemies)
        sprites.push_back({length(enemy.pos - player.pos), &enemy, 0, 0, 0});
    for (const auto& pickup : level.pickups)
        if (pickup.active)
            sprites.push_back({length(pickup.pos - player.pos),
                               nullptr,
                               pickup.pos.x,
                               pickup.pos.y,
                               pickup.type == PickupType::Health              ? 1
                               : pickup.type == PickupType::Ammo              ? 2
                               : pickup.type == PickupType::Key               ? 3
                               : pickup.type == PickupType::FlashlightBattery ? 4
                                                                              : 0});
    std::sort(sprites.begin(), sprites.end(), [](const auto& a, const auto& b) {
        return a.distance > b.distance;
    });
    for (const auto& item : sprites) {
        if (item.enemy)
            enemySprite(*item.enemy, player, debugEnemies);
        else
            worldSprite(item.x, item.y, item.kind, player, depth_);
    }

    for (const auto& projectile : projectiles) {
        const Vec2 delta = projectile.pos - player.pos;
        const float distance = length(delta);
        const float angle = wrapAngle(std::atan2(delta.y, delta.x) - visualAngle);
        if (std::abs(angle) > FOV * .65f || distance < .1f)
            continue;
        const int sx = static_cast<int>(W * .5f + std::tan(angle) / std::tan(FOV * .5f) * W * .5f);
        if (sx < 0 || sx >= W || distance > depth_[sx] + .2f)
            continue;
        const int base = H / 2 + static_cast<int>(H / (distance * .9f)) * .18f;
        const int size = projectile.exploding
                             ? static_cast<int>(28 + projectile.explosionTime * 130.f)
                             : std::clamp(static_cast<int>(26.f / distance), 5, 22);
        rect(sx - size,
             base - size,
             size * 2,
             size * 2,
             projectile.exploding ? 0xff167f91u : 0xff12636fu);
        rect(sx - size / 2, base - size / 2, size, size, 0xff37e8e1u);
        rect(sx - size / 4,
             base - size / 4,
             std::max(2, size / 2),
             std::max(2, size / 2),
             0xffe8ffffu);
        if (!projectile.exploding) {
            const Vec2 trail = normalized(projectile.velocity) * -.22f;
            for (int i = 1; i < 4; ++i)
                rect(sx + static_cast<int>(trail.x * i * 18),
                     base + static_cast<int>(trail.y * i * 8),
                     std::max(2, size / 3),
                     std::max(2, size / 3),
                     0xff259aa2u);
        }
    }

    for (const auto& impact : gun.impacts()) {
        const Vec2 delta = impact.pos - player.pos;
        const float distance = length(delta);
        const float angle = wrapAngle(std::atan2(delta.y, delta.x) - visualAngle);
        if (std::abs(angle) > FOV * .58f || distance < .15f)
            continue;
        const int sx = static_cast<int>(W * .5f + std::tan(angle) / std::tan(FOV * .5f) * W * .5f);
        if (sx < 0 || sx >= W || distance > depth_[sx] + .3f)
            continue;
        const int sy = H / 2;
        const float life = 1.f - impact.age / .38f;
        if (impact.kind == ImpactKind::Blood) {
            for (int i = 0; i < 5; ++i) {
                const int ox =
                    static_cast<int>(std::sin(impact.seed * 9.f + i * 2.1f) * 18.f * (1.f - life));
                const int oy = static_cast<int>(i * 5 + impact.age * 45.f);
                rect(sx + ox, sy - oy, 4, 4, i & 1 ? 0xffb62432u : 0xff6f1724u);
            }
        } else {
            for (int i = 0; i < 4; ++i) {
                const int ox = static_cast<int>(std::sin(impact.seed + i) * 24.f * impact.age);
                const int oy =
                    static_cast<int>(std::cos(impact.seed + i * 3.f) * 18.f * impact.age);
                rect(sx + ox, sy + oy, 3, 3, i ? 0xffffa33au : 0xffffeaa0u);
            }
            rect(sx - 4, sy - 4, 8, 8, 0xff3c3534u);
        }
    }

    const float move = player.movementAmount();
    const float bobSpeed = player.running() ? 12.f : 8.f;
    const float bobX = std::sin(time_ * bobSpeed) * 10.f * move;
    const float bobY =
        std::abs(std::cos(time_ * bobSpeed)) * (player.running() ? 8.f : 5.f) * move +
        std::sin(time_ * 1.7f) * 1.5f;
    const float recoilDown =
        gun.state() == WeaponState::Recoil    ? 22.f * (1.f - std::min(1.f, gun.animation() / .12f))
        : gun.state() == WeaponState::Recover ? 12.f * (1.f - std::min(1.f, gun.animation() / .28f))
                                              : 0.f;
    // Dim weapon sprite when flashlight is off (dark atmosphere)
    const float weaponBrightness = player.flashlightOn ? 1.0f : 0.35f;
    if (shotgunSprites_.valid())
        shotgunSprites_.draw(pixels_,
                             W,
                             H,
                             gun.spriteFrame(),
                             static_cast<int>(W / 2 - 145 + bobX),
                             static_cast<int>(100 + bobY + recoilDown),
                             290,
                             270,
                             weaponBrightness);
    if (gun.state() == WeaponState::Recoil || gun.state() == WeaponState::Recover) {
        const float t =
            gun.state() == WeaponState::Recoil ? gun.animation() : .12f + gun.animation();
        const int shellX = static_cast<int>(405 + t * 210.f);
        const int shellY = static_cast<int>(250 - t * 190.f + t * t * 260.f);
        rect(shellX, shellY, 9, 4, 0xffd6973au);
        rect(shellX + 2, shellY, 3, 4, 0xffffd267u);
    }

    rect(0, 326, W, 34, 0xff11131au);
    rect(0, 326, W, 3, 0xff53f6d0u);
    rect(386, 314, 52, 46, 0xff080a0fu);
    rect(388, 316, 48, 44, 0xff26313au);
    faceSprites_.drawCell(pixels_,
                          W,
                          H,
                          hudFace.column(),
                          hudFace.healthRow(player),
                          4,
                          390,
                          316,
                          44,
                          44,
                          1.f,
                          false);
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "HEALTH:%03d", player.health);
    text(12, 340, buffer, 0xfff1e8c9u, 2);
    std::snprintf(buffer, sizeof(buffer), "AMMO:%02d", player.ammo);
    text(148, 340, buffer, 0xffffd65au, 2);
    text(248,
         340,
         player.hasKey ? "KEY:CYAN" : "KEY:----",
         player.hasKey ? 0xff53f6d0u : 0xff555555u,
         2);

    // -----------------------------------------------------------------------
    // Flashlight HUD indicator
    // -----------------------------------------------------------------------
    {
        const float charge = player.flashlightCharge;
        const float pct = charge / Player::MaxFlashlightCharge;
        const bool empty = charge <= 0.f;
        const bool on = player.flashlightOn;

        // Label
        const std::uint32_t labelColor = empty         ? 0xffff3030u
                                         : !on         ? 0xff666666u
                                         : pct < 0.15f ? 0xffff4040u
                                         : pct < 0.35f ? 0xffff9900u
                                                       : 0xff53f6d0u;
        if (empty) {
            text(355, 330, "FL:EMPTY", 0xffff3030u, 2);
        } else {
            text(355, 330, on ? "FL:ON " : "FL:OFF", labelColor, 2);
        }

        // Battery bar: 80px wide, 6px tall at x=355, y=342
        const int barX = 355, barY = 342;
        const int barW = 80, barH = 6;
        // Background
        rect(barX - 1, barY - 1, barW + 2, barH + 2, 0xff222228u);
        rect(barX, barY, barW, barH, 0xff111115u);
        if (!empty) {
            const int filled = static_cast<int>(pct * barW);
            const std::uint32_t barColor = pct < 0.15f   ? 0xffcc1111u
                                           : pct < 0.35f ? 0xffcc8800u
                                                         : 0xff33cc88u;
            rect(barX, barY, filled, barH, shade(barColor, on ? 1.f : 0.55f));
        }
        // Percentage text
        const int pctVal = static_cast<int>(pct * 100.f);
        std::snprintf(buffer, sizeof(buffer), "%3d%%", pctVal);
        text(barX + barW + 4, barY, buffer, labelColor, 1);
    }
    if (player.hurtFlash > 0.0f)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (((x + y) & 5) == 0)
                    pixels_[y * W + x] = 0xff8d2028u;
    if (won) {
        rect(70, 115, 500, 115, 0xee10131cu);
        text(144, 140, "SECTOR SECURED", 0xff53f6d0u, 4);
        text(160, 195, "PRESS ESC TO QUIT", 0xfff1e8c9u, 2);
    }
    drawNotifications();
    present();
}

void Renderer::drawTitleScreen(int selection, float dt) {
    time_ += dt;
    // Clear screen
    std::fill(pixels_.begin(), pixels_.end(), 0xff000000u);

    // Draw background if loaded
    if (titleBgSprite_.valid()) {
        titleBgSprite_.drawCell(pixels_, W, H, 0, 0, 1, 0, 0, W, H, 1.0f);
    }

    // --- Procedural Animations ---
    
    // 1. Zombie Silhouette Breathing (Distortion)
    // The silhouette is roughly around x=490 to 570, y=160 to 280
    // We apply a slow, subtle horizontal wave only to the very dark pixels in this region
    float breathWave = std::sin(time_ * 1.8f);
    float swayWave = std::sin(time_ * 0.5f);
    for (int y = 160; y < 280; ++y) {
        for (int x = 490; x < 570; ++x) {
            std::uint32_t px = pixels_[y * W + x];
            unsigned r = (px >> 16u) & 255u;
            unsigned g = (px >> 8u) & 255u;
            unsigned b = px & 255u;
            // Only affect dark silhouette pixels (luma < 40)
            if (r + g + b < 120) {
                // Calculate horizontal shift based on height and time
                float shift = (y - 160) * 0.015f * breathWave + swayWave * 1.2f;
                int srcX = std::clamp(static_cast<int>(x + shift), 0, W - 1);
                pixels_[y * W + x] = pixels_[y * W + srcX];
            }
        }
    }

    // 2. Emergency Light Flicker
    // The red light is roughly around x=400 to 430, y=120 to 135
    // Random flicker pattern
    bool flicker = (std::fmod(std::sin(time_ * 12.3f) + std::sin(time_ * 4.7f), 1.0f) > 0.8f);
    if (flicker) {
        for (int y = 115; y < 145; ++y) {
            for (int x = 390; x < 440; ++x) {
                std::uint32_t px = pixels_[y * W + x];
                unsigned r = (px >> 16u) & 255u;
                unsigned g = (px >> 8u) & 255u;
                unsigned b = px & 255u;
                // Dim the light slightly
                r = static_cast<unsigned>(r * 0.6f);
                g = static_cast<unsigned>(g * 0.6f);
                b = static_cast<unsigned>(b * 0.6f);
                pixels_[y * W + x] = 0xff000000u | (r << 16u) | (g << 8u) | b;
            }
        }
    }

    // 3. Update & Draw Particles (Steam and Sparks)
    if (time_ > nextSparkTime_) {
        // Spawn a spark
        TitleParticle p;
        p.x = 220.0f + (std::rand() % 10);
        p.y = 120.0f;
        p.vx = -15.0f + (std::rand() % 30);
        p.vy = -20.0f - (std::rand() % 40);
        p.life = p.maxLife = 0.5f + (std::rand() % 10) * 0.05f;
        p.type = 1;
        titleParticles_.push_back(p);
        nextSparkTime_ = time_ + 2.0f + (std::rand() % 30) * 0.1f;
    }
    
    // Periodically spawn steam
    if (std::rand() % 10 == 0 && titleParticles_.size() < 25) {
        TitleParticle p;
        p.x = 480.0f + (std::rand() % 60);
        p.y = 280.0f + (std::rand() % 20);
        p.vx = -5.0f + (std::rand() % 10);
        p.vy = -15.0f - (std::rand() % 15);
        p.life = p.maxLife = 2.0f + (std::rand() % 20) * 0.1f;
        p.type = 0; // steam
        titleParticles_.push_back(p);
    }

    for (auto it = titleParticles_.begin(); it != titleParticles_.end(); ) {
        it->life -= dt;
        if (it->life <= 0) {
            it = titleParticles_.erase(it);
            continue;
        }
        
        it->x += it->vx * dt;
        it->y += it->vy * dt;
        
        if (it->type == 1) { // Spark
            it->vy += 200.0f * dt; // Gravity
            int ix = static_cast<int>(it->x);
            int iy = static_cast<int>(it->y);
            if (ix >= 0 && ix < W && iy >= 0 && iy < H) {
                float intensity = it->life / it->maxLife;
                unsigned c = static_cast<unsigned>(255.0f * intensity);
                pixels_[iy * W + ix] = 0xff000000u | (255u << 16u) | (200u << 8u) | c;
            }
        } else if (it->type == 0) { // Steam
            it->vx += (std::sin(time_ * 2.0f + it->y * 0.05f) * 10.0f) * dt; // Drift
            float alpha = std::sin((it->life / it->maxLife) * 3.14159f) * 0.12f;
            
            int size = 12;
            int startX = static_cast<int>(it->x) - size;
            int startY = static_cast<int>(it->y) - size;
            
            for (int sy = 0; sy < size * 2; ++sy) {
                for (int sx = 0; sx < size * 2; ++sx) {
                    float dist = std::sqrt(static_cast<float>((sx - size) * (sx - size) + (sy - size) * (sy - size)));
                    if (dist > size) continue;
                    
                    int pxX = startX + sx;
                    int pxY = startY + sy;
                    
                    if (pxX >= 0 && pxX < W && pxY >= 0 && pxY < H) {
                        float pixelAlpha = alpha * (1.0f - dist / size);
                        std::uint32_t bg = pixels_[pxY * W + pxX];
                        unsigned r = (bg >> 16u) & 255u;
                        unsigned g = (bg >> 8u) & 255u;
                        unsigned b = bg & 255u;
                        
                        r = static_cast<unsigned>(r * (1.0f - pixelAlpha) + 200.0f * pixelAlpha);
                        g = static_cast<unsigned>(g * (1.0f - pixelAlpha) + 200.0f * pixelAlpha);
                        b = static_cast<unsigned>(b * (1.0f - pixelAlpha) + 220.0f * pixelAlpha);
                        
                        pixels_[pxY * W + pxX] = 0xff000000u | (r << 16u) | (g << 8u) | b;
                    }
                }
            }
        }
        ++it;
    }

    // Apply CRT/Vignette effect (darken corners, slight scanline)
    for (int y = 0; y < H; ++y) {
        float dy = (y - H / 2.0f) / (H / 2.0f);
        for (int x = 0; x < W; ++x) {
            float dx = (x - W / 2.0f) / (W / 2.0f);
            float dist2 = dx * dx + dy * dy;
            float vignette = std::clamp(1.2f - dist2 * 0.8f, 0.0f, 1.0f);
            float scanline = (y % 2 == 0) ? 0.9f : 1.0f;
            float brightness = vignette * scanline;
            
            auto& px = pixels_[y * W + x];
            unsigned r = ((px >> 16u) & 255u) * brightness;
            unsigned g = ((px >> 8u) & 255u) * brightness;
            unsigned b = (px & 255u) * brightness;
            px = 0xff000000u | (r << 16u) | (g << 8u) | b;
        }
    }

    // VOIDLOCK Title Text
    // Corrupted red/orange color with a slight pulse
    float pulse = (std::sin(time_ * 3.0f) + 1.0f) * 0.5f;
    unsigned titleR = 180 + static_cast<unsigned>(40 * pulse);
    unsigned titleG = 20 + static_cast<unsigned>(10 * pulse);
    unsigned titleB = 20;
    std::uint32_t titleColor = 0xff000000u | (titleR << 16u) | (titleG << 8u) | titleB;
    
    text(80, 50, "VOID", titleColor, 6);
    text(80 + 4 * 6 * 8, 50, "LOCK", 0xff662222u, 6); // darker, corrupted look

    // Menu Options
    const char* options[] = {"START GAME", "CONTINUE", "SETTINGS", "EXIT"};
    int startY = 180;
    for (int i = 0; i < 4; ++i) {
        std::uint32_t color = 0xff888888u; // dim grey
        if (i == selection) {
            color = 0xffdd4444u; // bright red when selected
            text(60, startY + i * 30, ">", color, 2);
        }
        text(80, startY + i * 30, options[i], color, 2);
    }
    
    text(W - 80, H - 20, "v1.0", 0xff444444u, 1);
    
    present();
}

void Renderer::drawTransition(float progress) {
    // Fade to black based on progress (1.0 to 0.0)
    for (auto& px : pixels_) {
        unsigned r = ((px >> 16u) & 255u) * progress;
        unsigned g = ((px >> 8u) & 255u) * progress;
        unsigned b = (px & 255u) * progress;
        px = 0xff000000u | (r << 16u) | (g << 8u) | b;
    }
    present();
}

void Renderer::drawLoadingScreen(float dt) {
    time_ += dt;
    std::fill(pixels_.begin(), pixels_.end(), 0xff050505u); // Almost black
    
    // Add subtle scanlines
    for (int y = 0; y < H; y += 2) {
        for (int x = 0; x < W; ++x) {
            pixels_[y * W + x] = 0xff020202u;
        }
    }

    text(80, H / 2 - 20, "INITIALIZING CONTAINMENT SYSTEM...", 0xff555555u, 2);
    
    // Simple pixel progress bar
    int barWidth = 300;
    int progressWidth = static_cast<int>(barWidth * std::fmod(time_ * 2.0f, 1.0f));
    
    rect(80, H / 2 + 10, barWidth, 10, 0xff222222u);
    rect(80, H / 2 + 10, progressWidth, 10, 0xff662222u);
    
    text(80, H / 2 + 40, "PLEASE WAIT", 0xff333333u, 1);
    
    present();
}

void Renderer::pushNotification(const char* msg, std::uint32_t color) {
    // Shift existing notes up if full
    if (noteCount_ >= MaxNotes) {
        for (int i = 0; i < MaxNotes - 1; ++i)
            notes_[i] = notes_[i + 1];
        noteCount_ = MaxNotes - 1;
    }
    auto& n = notes_[noteCount_++];
    std::strncpy(n.text, msg, sizeof(n.text) - 1);
    n.text[sizeof(n.text) - 1] = '\0';
    n.timer = 0.f;
    n.maxTimer = 2.4f;
    n.color = color;
}

void Renderer::drawNotifications() {
    // Update timers (use dt if available; approximate with frame time)
    // We update here using the stored notes; timer is advanced each frame at ~60fps
    // We use time_ delta approximation — advance by fixed frame time estimate
    // (Proper solution: pass dt to draw(); this is a simple approach)
    static float lastTime = 0.f;
    const float dt = std::min(0.05f, time_ - lastTime);
    lastTime = time_;
    // Advance timers for all active notifications
    for (int i = 0; i < noteCount_; ++i)
        notes_[i].timer += dt;
    // Compact (remove expired)
    int out = 0;
    for (int i = 0; i < noteCount_; ++i)
        if (notes_[i].timer < notes_[i].maxTimer)
            notes_[out++] = notes_[i];
    noteCount_ = out;
    // Draw from oldest (bottom) to newest (top)
    for (int i = 0; i < noteCount_; ++i) {
        const auto& n = notes_[i];
        const float life = 1.f - n.timer / n.maxTimer;
        // Fade in over 0.15s, fade out over last 0.5s
        float alpha = 1.f;
        if (n.timer < 0.15f)
            alpha = n.timer / 0.15f;
        else if (n.timer > n.maxTimer - 0.5f)
            alpha = (n.maxTimer - n.timer) / 0.5f;
        (void)life;
        // Gentle slide-in from the right
        const float slide = (n.timer < 0.15f) ? (1.f - n.timer / 0.15f) * 40.f : 0.f;
        const int baseY = H - 40 - i * 14; // stack upward above HUD
        const int baseX = static_cast<int>(W - 170 + slide);
        // Semi-transparent background strip
        const int len = static_cast<int>(std::strlen(n.text));
        const int stripW = len * 6 + 10;
        for (int yy = baseY - 1; yy < baseY + 9; ++yy)
            for (int xx = baseX - 4; xx < baseX + stripW; ++xx) {
                if (xx < 0 || xx >= W || yy < 0 || yy >= H)
                    continue;
                const auto& px = pixels_[yy * W + xx];
                const unsigned rr = (px >> 16u) & 255u;
                const unsigned gg = (px >> 8u) & 255u;
                const unsigned bb = px & 255u;
                const float a = alpha * 0.68f;
                pixels_[yy * W + xx] = 0xff000000u | (static_cast<unsigned>(rr * (1 - a)) << 16u) |
                                       (static_cast<unsigned>(gg * (1 - a)) << 8u) |
                                       static_cast<unsigned>(bb * (1 - a));
            }
        // Colorized text
        const unsigned rr = (n.color >> 16u) & 255u;
        const unsigned gg = (n.color >> 8u) & 255u;
        const unsigned bb = n.color & 255u;
        const std::uint32_t fadedColor = 0xff000000u | (static_cast<unsigned>(rr * alpha) << 16u) |
                                         (static_cast<unsigned>(gg * alpha) << 8u) |
                                         static_cast<unsigned>(bb * alpha);
        text(baseX, baseY, n.text, fadedColor, 1);
    }
}

void Renderer::present() {
    static bool captured = false;
    static int captureFrames = 0;
    if (!captured && captureFrames++ > 12) {
        if (const char* path = std::getenv("VOIDLOCK_CAPTURE_FRAME")) {
            SDL_Surface* surface =
                SDL_CreateSurfaceFrom(W,
                                      H,
                                      SDL_PIXELFORMAT_ARGB8888,
                                      pixels_.data(),
                                      W * static_cast<int>(sizeof(std::uint32_t)));
            if (surface) {
                SDL_SaveBMP(surface, path);
                SDL_DestroySurface(surface);
                captured = true;
            }
        }
    }
    int windowW = 0, windowH = 0;
    SDL_GetWindowSizeInPixels(window_, &windowW, &windowH);
    int viewportW = windowW;
    int viewportH = viewportW * H / W;
    if (viewportH > windowH) {
        viewportH = windowH;
        viewportW = viewportH * W / H;
    }
    const int integerScale = std::min(windowW / W, windowH / H);
    if (integerScale >= 1) {
        viewportW = W * integerScale;
        viewportH = H * integerScale;
    }
    const int viewportX = (windowW - viewportW) / 2;
    const int viewportY = (windowH - viewportH) / 2;

    glViewport(0, 0, windowW, windowH);
    glClearColor(0.015f, 0.018f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(viewportX, viewportY, viewportW, viewportH);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_BGRA, GL_UNSIGNED_BYTE, pixels_.data());
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(0, 0);
    glVertex2f(-1, 1);
    glEnd();
    SDL_GL_SwapWindow(window_);
}
