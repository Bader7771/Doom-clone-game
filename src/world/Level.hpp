#pragma once
#include "game/Types.hpp"
#include <string>
#include <vector>

enum class PickupType { Health, Ammo, Key };
struct Pickup { Vec2 pos; PickupType type; bool active{true}; };

class Level {
public:
  Level();
  int width() const { return w_; }
  int height() const { return h_; }
  char tile(int x,int y) const;
  bool solid(float x,float y) const;
  bool tryOpenDoor(Vec2 player, Vec2 facing, bool hasKey);
  bool lineClear(Vec2 a, Vec2 b) const;
  bool atExit(Vec2 p) const;
  std::vector<Pickup> pickups;
  std::vector<Vec2> enemySpawns;
private:
  int w_{},h_{};
  std::vector<std::string> grid_;
};

