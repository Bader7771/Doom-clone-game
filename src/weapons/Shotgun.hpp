#pragma once
class Player; class Level; class Audio; class Enemy;
#include <vector>
class Shotgun {
public:
  void update(float dt,bool trigger,Player&,const Level&,std::vector<Enemy>&,Audio&);
  float animation() const { return cooldown_; }
  float flash() const { return flash_; }
private: float cooldown_{},flash_{};
};

