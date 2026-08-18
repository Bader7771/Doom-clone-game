#pragma once
#include <cstdint>
#include <vector>

class Player; class Shotgun; class Enemy;

enum class FaceState { ForwardA, ForwardB, Blink, LookLeft, LookRight, Attack, PainFront, PainLeft, PainRight, Kill, Dead };

class HudFace {
public:
  void update(float dt,const Player&,const Shotgun&,const std::vector<Enemy>&);
  int column() const;
  int healthRow(const Player&) const;
  FaceState state() const{return state_;}
private:
  void react(FaceState state,float duration,int priority);
  FaceState state_{FaceState::ForwardA};
  float reactionTime_{},idleTime_{},lookHold_{};
  int priority_{},knownDeaths_{};
  std::uint32_t damageSerial_{};
  bool firing_{};
};
