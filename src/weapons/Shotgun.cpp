#include "weapons/Shotgun.hpp"
#include "audio/Audio.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "world/Level.hpp"
#include <algorithm>
#include <cmath>

namespace { constexpr float PelletSpread=.075f; }

int Shotgun::spriteFrame() const {
  switch(state_){case WeaponState::Fire:return 2;case WeaponState::Recoil:return 3;case WeaponState::Recover:return stateTime_<.1f?4:5;default:return stateTime_<.08f?1:0;}
}

void Shotgun::update(float dt,bool trigger,Player& player,const Level& level,std::vector<Enemy>& enemies,Audio& audio){
  cooldown_=std::max(0.f,cooldown_-dt);flash_=std::max(0.f,flash_-dt);stateTime_+=dt;
  for(auto& impact:impacts_)impact.age+=dt;
  std::erase_if(impacts_,[](const ShotImpact& impact){return impact.age>.38f;});
  if(state_==WeaponState::Fire&&stateTime_>.055f){state_=WeaponState::Recoil;stateTime_=0;}
  else if(state_==WeaponState::Recoil&&stateTime_>.12f){state_=WeaponState::Recover;stateTime_=0;}
  else if(state_==WeaponState::Recover&&stateTime_>.28f){state_=player.movementAmount()>.15f?(player.running()?WeaponState::Run:WeaponState::Walk):WeaponState::Idle;stateTime_=0;}
  else if(state_==WeaponState::Idle||state_==WeaponState::Walk||state_==WeaponState::Run){state_=player.movementAmount()>.15f?(player.running()?WeaponState::Run:WeaponState::Walk):WeaponState::Idle;}
  if(trigger&&cooldown_<=0&&player.ammo>0&&player.health>0)fire(player,level,enemies,audio);
}

void Shotgun::fire(Player& player,const Level& level,std::vector<Enemy>& enemies,Audio& audio){
  --player.ammo;cooldown_=.72f;flash_=.075f;state_=WeaponState::Fire;stateTime_=0;shotSeed_+=1.f;
  player.addWeaponKick(.055f,.72f);audio.playShot();
  std::vector<int> damage(enemies.size());
  for(int pellet=0;pellet<11;++pellet){
    const float spread=(pellet-5)*PelletSpread/5.f+std::sin(shotSeed_*12.989f+pellet*4.17f)*.012f;
    const float angle=player.angle+spread;const Vec2 direction{std::cos(angle),std::sin(angle)};
    float hitDistance=12.f;int hitEnemy=-1;
    for(std::size_t i=0;i<enemies.size();++i){if(enemies[i].state==EnemyState::Dead)continue;const Vec2 relative=enemies[i].pos-player.pos;const float along=dot(relative,direction);const float side=length(relative-direction*along);if(along>.1f&&along<hitDistance&&side<.32f&&level.lineClear(player.pos,enemies[i].pos)){hitDistance=along;hitEnemy=static_cast<int>(i);}}
    Vec2 hit=player.pos+direction*hitDistance;
    if(hitEnemy<0){for(float d=.1f;d<12.f;d+=.06f){const Vec2 point=player.pos+direction*d;if(level.solid(point.x,point.y)){hit=point;break;}}impacts_.push_back({hit,ImpactKind::Stone,0,shotSeed_+pellet});}
    else {damage[hitEnemy]+=5;impacts_.push_back({enemies[hitEnemy].pos,ImpactKind::Blood,0,shotSeed_+pellet});}
  }
  const Vec2 forward{std::cos(player.angle),std::sin(player.angle)};
  for(std::size_t i=0;i<enemies.size();++i)if(damage[i]>0)enemies[i].damage(damage[i],forward,damage[i]>=25?1.f:.45f);
}
