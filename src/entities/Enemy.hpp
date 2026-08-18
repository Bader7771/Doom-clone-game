#pragma once
#include "game/Types.hpp"
class Player; class Level;
enum class EnemyState { Idle, Chase, Attack, Dead };
class Enemy {
public:
  explicit Enemy(Vec2 p):pos(p){}
  void update(float dt,Player&,const Level&);
  void damage(int amount);
  Vec2 pos; EnemyState state{EnemyState::Idle}; int health{40}; float anim{};
private: float attackCooldown{};
};

