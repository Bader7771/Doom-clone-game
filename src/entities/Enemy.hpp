#pragma once
#include "game/Types.hpp"
#include <vector>

class Player; class Level; class Audio;

enum class EnemyType : int { Rusher, Gunner, Brute };
enum class EnemyState { Idle, Alert, Chase, Attack, Pain, Dying, Dead };
enum class EnemyAnimation { Idle, Alert, Walk, Run, Attack, Pain, Death };

struct EnemyProjectile {
  Vec2 pos{}, velocity{};
  float life{4.f}, explosionTime{}, light{1.f};
  int damage{24};
  bool exploding{}, damageApplied{};
};

struct EnemyDefinition {
  const char* name;
  int health;
  float speed, radius, visualScale, detectionRange, preferredRange;
};

class Enemy {
public:
  Enemy(EnemyType type, Vec2 position);
  void update(float dt, Player&, const Level&, std::vector<EnemyProjectile>&, Audio&);
  void damage(int amount, Vec2 direction = {}, float force = 0.0f);
  EnemyAnimation animation() const;
  int animationFrame() const;
  int directionFrame(const Player&) const;
  float animationRate() const;
  const EnemyDefinition& definition() const;
  const char* stateName() const;

  EnemyType type;
  Vec2 pos, facing{1.f,0.f};
  EnemyState state{EnemyState::Idle};
  int health{};
  float stateTime{}, animTime{}, painFlash{}, muzzleFlash{}, movedSpeed{};

private:
  void enter(EnemyState next);
  bool move(Vec2 velocity, float dt, const Level&);
  void updateRusher(float dt, Player&, const Level&, Audio&);
  void updateGunner(float dt, Player&, const Level&, Audio&);
  void updateBrute(float dt, Player&, const Level&, std::vector<EnemyProjectile>&, Audio&);
  float attackCooldown_{}, strafeSign_{1.f};
  unsigned attackFlags_{};
};
