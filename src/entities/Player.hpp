#pragma once
#include "game/Types.hpp"
#include <cstdint>
class Input;
class Level;

class Player {
  public:
    void update(float dt, const Input&, const Level&);
    void hurt(int amount, Vec2 source = {});
    void addWeaponKick(float recoil, float shake);
    Vec2 pos{2.5f, 2.5f};
    float angle{};
    int health{100}, ammo{12};
    bool hasKey{false};
    float hurtFlash{};
    float movementAmount() const {
        return movementAmount_;
    }
    bool running() const {
        return running_;
    }
    float viewKick() const {
        return viewKick_;
    }
    float screenShake() const {
        return screenShake_;
    }
    float lateralMovement() const {
        return lateralMovement_;
    }
    std::uint32_t damageSerial() const {
        return damageSerial_;
    }
    int lastDamage() const {
        return lastDamage_;
    }
    float damageSide() const {
        return damageSide_;
    }

  private:
    void move(Vec2 delta, const Level&);
    float movementAmount_{}, viewKick_{}, screenShake_{}, lateralMovement_{}, damageSide_{};
    std::uint32_t damageSerial_{};
    int lastDamage_{};
    bool running_{};
};
