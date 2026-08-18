#include "ui/HudFace.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "weapons/Shotgun.hpp"
#include <algorithm>
#include <cmath>

void HudFace::react(FaceState next,float duration,int priority){if(priority<priority_&&reactionTime_>0)return;state_=next;reactionTime_=duration;priority_=priority;}

void HudFace::update(float dt,const Player& player,const Shotgun& shotgun,const std::vector<Enemy>& enemies){
  idleTime_+=dt;reactionTime_=std::max(0.f,reactionTime_-dt);lookHold_=std::max(0.f,lookHold_-dt);if(reactionTime_<=0)priority_=0;
  const int deaths=static_cast<int>(std::count_if(enemies.begin(),enemies.end(),[](const Enemy& enemy){return enemy.state==EnemyState::Dying||enemy.state==EnemyState::Dead;}));
  if(player.health<=0){state_=FaceState::Dead;reactionTime_=999.f;priority_=100;knownDeaths_=deaths;return;}
  if(state_==FaceState::Dead){reactionTime_=0;priority_=0;state_=FaceState::ForwardA;}
  if(player.damageSerial()!=damageSerial_){damageSerial_=player.damageSerial();const FaceState pain=player.damageSide()<-.25f?FaceState::PainLeft:player.damageSide()>.25f?FaceState::PainRight:FaceState::PainFront;react(pain,player.lastDamage()>=24?.7f:player.lastDamage()>=10?.5f:.34f,80);}
  if(deaths>knownDeaths_)react(FaceState::Kill,.42f,55);knownDeaths_=deaths;
  const bool firing=shotgun.state()==WeaponState::Fire;if(firing&&!firing_)react(FaceState::Attack,.2f,45);firing_=firing;
  if(reactionTime_>0)return;
  const float lateral=player.lateralMovement();
  if(lateral<-.25f){state_=FaceState::LookLeft;lookHold_=.18f;return;}if(lateral>.25f){state_=FaceState::LookRight;lookHold_=.18f;return;}
  if(lookHold_>0)return;
  const float cycle=std::fmod(idleTime_,4.1f);if(cycle<.11f)state_=FaceState::Blink;else state_=(static_cast<int>(idleTime_/2.05f)&1)?FaceState::ForwardB:FaceState::ForwardA;
}

int HudFace::column()const{switch(state_){case FaceState::ForwardA:return 0;case FaceState::ForwardB:return 1;case FaceState::Blink:return 2;case FaceState::LookLeft:return 3;case FaceState::LookRight:return 4;case FaceState::Attack:return 5;case FaceState::PainFront:return 6;case FaceState::PainLeft:return 7;case FaceState::PainRight:return 8;case FaceState::Kill:return 9;case FaceState::Dead:return 9;}return 0;}
int HudFace::healthRow(const Player& player)const{if(state_==FaceState::Dead)return 3;if(player.health>75)return 0;if(player.health>50)return 1;if(player.health>25)return 2;return 3;}
