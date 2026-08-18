#include "entities/Enemy.hpp"
#include "audio/Audio.hpp"
#include "entities/Player.hpp"
#include "world/Level.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr std::array<EnemyDefinition, 3> definitions{
    {{"RUSHER", 36, 2.15f, .18f, .65f, 9.f, .75f},
     {"GUNNER", 72, .82f, .24f, .76f, 11.f, 4.7f},
     {"BRUTE", 190, .47f, .30f, .84f, 12.f, 6.2f}}};
}

Enemy::Enemy(EnemyType enemyType, Vec2 position)
    : type(enemyType), pos(position), health(definition().health) {
    strafeSign_ = std::fmod(position.x + position.y, 2.f) > .9f ? -1.f : 1.f;
}
const EnemyDefinition& Enemy::definition() const {
    return definitions[static_cast<std::size_t>(type)];
}
void Enemy::enter(EnemyState next) {
    if (state == next)
        return;
    state = next;
    stateTime = 0;
    animTime = 0;
    attackFlags_ = 0;
}

bool Enemy::move(Vec2 velocity, float dt, const Level& level) {
    Vec2 next = pos + velocity * dt;
    bool changed = false;
    const float r = definition().radius;
    if (!level.solid(next.x + (velocity.x > 0 ? r : -r), pos.y)) {
        pos.x = next.x;
        changed = true;
    }
    if (!level.solid(pos.x, next.y + (velocity.y > 0 ? r : -r))) {
        pos.y = next.y;
        changed = true;
    }
    if (changed && length(velocity) > .01f)
        facing = normalized(velocity);
    movedSpeed = changed ? length(velocity) : 0;
    return changed;
}

void Enemy::damage(int amount, Vec2 direction, float force) {
    if (state == EnemyState::Dead || state == EnemyState::Dying)
        return;
    health -= amount;
    painFlash = .15f;
    const float resistance = type == EnemyType::Brute    ? .32f
                             : type == EnemyType::Gunner ? .72f
                                                         : 1.f;
    pos += direction * (force * .18f * resistance);
    if (health <= 0) {
        enter(EnemyState::Dying);
        return;
    }
    if (type != EnemyType::Brute || amount >= 25)
        enter(EnemyState::Pain);
}

EnemyAnimation Enemy::animation() const {
    switch (state) {
    case EnemyState::Idle:
        return EnemyAnimation::Idle;
    case EnemyState::Alert:
        return EnemyAnimation::Alert;
    case EnemyState::Chase:
        return type == EnemyType::Rusher ? EnemyAnimation::Run : EnemyAnimation::Walk;
    case EnemyState::Attack:
        return EnemyAnimation::Attack;
    case EnemyState::Pain:
        return EnemyAnimation::Pain;
    case EnemyState::Dying:
    case EnemyState::Dead:
        return EnemyAnimation::Death;
    }
    return EnemyAnimation::Idle;
}
float Enemy::animationRate() const {
    if (state == EnemyState::Chase)
        return std::max(.1f, movedSpeed) / (definition().speed) *
               (type == EnemyType::Rusher ? 9.f : 5.f);
    return 1.f;
}
int Enemy::animationFrame() const {
    if (state == EnemyState::Dead)
        return 7;
    if (state == EnemyState::Dying)
        return std::min(7, static_cast<int>(stateTime / 0.095f));
    if (state == EnemyState::Chase)
        return 2 + (static_cast<int>(animTime * animationRate()) & 1);
    if (state == EnemyState::Idle)
        return 0;
    if (state == EnemyState::Alert)
        return 1;
    if (state == EnemyState::Pain)
        return 6;
    if (state == EnemyState::Attack) {
        if (type == EnemyType::Rusher)
            return stateTime < .28f ? 4 : 5;
        if (type == EnemyType::Gunner)
            return stateTime < .42f ? 4 : 5;
        return stateTime < 1.05f ? 4 : 5;
    }
    return 0;
}
int Enemy::directionFrame(const Player& player) const {
    if (state == EnemyState::Dying || state == EnemyState::Dead)
        return animationFrame();
    const float view = std::atan2(player.pos.y - pos.y, player.pos.x - pos.x);
    const float face = std::atan2(facing.y, facing.x);
    float relative = wrapAngle(view - face);
    int direction =
        static_cast<int>(std::floor((relative + 3.14159265f / 8.f) / (3.14159265f / 4.f)));
    return (direction % 8 + 8) % 8;
}
const char* Enemy::stateName() const {
    switch (state) {
    case EnemyState::Idle:
        return "IDLE";
    case EnemyState::Alert:
        return "ALERT";
    case EnemyState::Chase:
        return "CHASE";
    case EnemyState::Attack:
        return "ATTACK";
    case EnemyState::Pain:
        return "PAIN";
    case EnemyState::Dying:
        return "DYING";
    case EnemyState::Dead:
        return "DEAD";
    }
    return "?";
}

void Enemy::updateRusher(float dt, Player& player, const Level& level, Audio& audio) {
    const Vec2 toPlayer = player.pos - pos;
    const float distance = length(toPlayer);
    const Vec2 forward = normalized(toPlayer), side{-forward.y, forward.x};
    if (state == EnemyState::Chase) {
        if (distance < 1.05f && attackCooldown_ <= 0) {
            enter(EnemyState::Attack);
            audio.playEnemyAttack(type);
            return;
        }
        const float weave = std::sin(animTime * 5.2f) * .22f;
        move(normalized(forward + side * weave) * definition().speed, dt, level);
    } else if (state == EnemyState::Attack) {
        facing = forward;
        if (stateTime > .28f && stateTime < .43f)
            move(forward * 4.1f, dt, level);
        if (stateTime >= .34f && !(attackFlags_ & 1u)) {
            attackFlags_ |= 1u;
            if (distance < 1.15f) {
                player.hurt(14, pos);
                player.addWeaponKick(.018f, .25f);
            }
        }
        if (stateTime > .68f) {
            attackCooldown_ = .42f;
            enter(EnemyState::Chase);
        }
    }
}

void Enemy::updateGunner(float dt, Player& player, const Level& level, Audio& audio) {
    const Vec2 toPlayer = player.pos - pos;
    const float distance = length(toPlayer);
    const Vec2 forward = normalized(toPlayer), side{-forward.y, forward.x};
    facing = forward;
    if (state == EnemyState::Chase) {
        if (distance > 3.6f && distance < 8.f && level.lineClear(pos, player.pos) &&
            attackCooldown_ <= 0) {
            enter(EnemyState::Attack);
            return;
        }
        float radial = distance > definition().preferredRange ? 1.f : distance < 3.1f ? -1.f : 0.f;
        move(normalized(forward * radial + side * strafeSign_ * .55f) * definition().speed,
             dt,
             level);
    } else if (state == EnemyState::Attack) {
        for (int shot = 0; shot < 3; ++shot) {
            const float moment = .48f + shot * .15f;
            if (stateTime >= moment && !(attackFlags_ & (1u << shot))) {
                attackFlags_ |= 1u << shot;
                muzzleFlash = .07f;
                audio.playEnemyAttack(type);
                if (level.lineClear(pos, player.pos) && distance < 9.f)
                    player.hurt(4, pos);
            }
        }
        if (stateTime > 1.12f) {
            attackCooldown_ = 1.05f;
            strafeSign_ = -strafeSign_;
            enter(EnemyState::Chase);
        }
    }
}

void Enemy::updateBrute(float dt,
                        Player& player,
                        const Level& level,
                        std::vector<EnemyProjectile>& projectiles,
                        Audio& audio) {
    const Vec2 toPlayer = player.pos - pos;
    const float distance = length(toPlayer);
    const Vec2 forward = normalized(toPlayer);
    facing = forward;
    if (state == EnemyState::Chase) {
        if (((distance < 1.25f) || (distance < 9.f && level.lineClear(pos, player.pos))) &&
            attackCooldown_ <= 0) {
            enter(EnemyState::Attack);
            audio.playEnemyCharge(type);
            return;
        }
        move(forward * definition().speed, dt, level);
    } else if (state == EnemyState::Attack) {
        const bool melee = distance < 1.55f;
        if (melee) {
            if (stateTime > .72f && !(attackFlags_ & 1u)) {
                attackFlags_ |= 1u;
                if (distance < 1.65f) {
                    player.hurt(30, pos);
                    player.addWeaponKick(.035f, .8f);
                }
            }
            if (stateTime > 1.35f) {
                attackCooldown_ = 1.2f;
                enter(EnemyState::Chase);
            }
        } else {
            if (stateTime > 1.05f && !(attackFlags_ & 1u)) {
                attackFlags_ |= 1u;
                muzzleFlash = .22f;
                projectiles.push_back({pos + forward * .5f, forward * 2.15f});
                audio.playEnemyAttack(type);
            }
            if (stateTime > 1.65f) {
                attackCooldown_ = 1.65f;
                enter(EnemyState::Chase);
            }
        }
    }
}

void Enemy::update(float dt,
                   Player& player,
                   const Level& level,
                   std::vector<EnemyProjectile>& projectiles,
                   Audio& audio) {
    stateTime += dt;
    animTime += dt;
    painFlash = std::max(0.f, painFlash - dt);
    muzzleFlash = std::max(0.f, muzzleFlash - dt);
    attackCooldown_ = std::max(0.f, attackCooldown_ - dt);
    movedSpeed = 0;
    if (state == EnemyState::Dead)
        return;
    if (state == EnemyState::Dying) {
        if (stateTime > .78f)
            enter(EnemyState::Dead);
        return;
    }
    if (state == EnemyState::Pain) {
        if (stateTime > .18f)
            enter(EnemyState::Chase);
        return;
    }
    const float distance = length(player.pos - pos);
    if (state == EnemyState::Idle && distance < definition().detectionRange &&
        level.lineClear(pos, player.pos)) {
        enter(EnemyState::Alert);
        audio.playEnemyAlert(type);
        return;
    }
    if (state == EnemyState::Alert) {
        facing = normalized(player.pos - pos);
        if (stateTime > (type == EnemyType::Rusher   ? .38f
                         : type == EnemyType::Gunner ? .62f
                                                     : .85f))
            enter(EnemyState::Chase);
        return;
    }
    if (type == EnemyType::Rusher)
        updateRusher(dt, player, level, audio);
    else if (type == EnemyType::Gunner)
        updateGunner(dt, player, level, audio);
    else
        updateBrute(dt, player, level, projectiles, audio);
}
