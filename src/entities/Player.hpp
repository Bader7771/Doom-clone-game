#pragma once
#include "game/Types.hpp"
class Input; class Level;

class Player {
public:
  void update(float dt,const Input&,const Level&);
  void hurt(int amount);
  Vec2 pos{2.5f,2.5f};
  float angle{};
  int health{100}, ammo{12};
  bool hasKey{false};
  float hurtFlash{};
private:
  void move(Vec2 delta,const Level&);
};

