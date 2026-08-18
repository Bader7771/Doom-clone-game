#pragma once
#include "game/Types.hpp"
#include <vector>

class Player; class Level; class Audio; class Enemy;

enum class WeaponState { Idle, Walk, Run, Fire, Recoil, Recover, Reload, Switch };
enum class ImpactKind { Metal, Stone, Blood };
struct ShotImpact { Vec2 pos; ImpactKind kind; float age{}; float seed{}; };

class Shotgun {
public:
  void update(float dt, bool trigger, Player&, const Level&, std::vector<Enemy>&, Audio&);
  float animation() const { return stateTime_; }
  float flash() const { return flash_; }
  float flashStrength() const { return flash_ / .075f; }
  int spriteFrame() const;
  WeaponState state() const { return state_; }
  const std::vector<ShotImpact>& impacts() const { return impacts_; }
private:
  void fire(Player&, const Level&, std::vector<Enemy>&, Audio&);
  WeaponState state_{WeaponState::Idle};
  float stateTime_{}, cooldown_{}, flash_{}, shotSeed_{};
  std::vector<ShotImpact> impacts_;
};
