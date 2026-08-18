#include "weapons/Shotgun.hpp"
#include "audio/Audio.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "world/Level.hpp"
#include <cmath>

void Shotgun::update(float dt,bool trigger,Player& p,const Level& l,std::vector<Enemy>& es,Audio& a){
  cooldown_=std::max(0.f,cooldown_-dt);flash_=std::max(0.f,flash_-dt);
  if(!trigger||cooldown_>0||p.ammo<=0||p.health<=0)return;
  --p.ammo;cooldown_=.72f;flash_=.09f;a.playShot();
  Vec2 f{std::cos(p.angle),std::sin(p.angle)};Enemy* best=nullptr;float bd=9.f;
  for(auto& e:es){if(e.state==EnemyState::Dead)continue;Vec2 d=e.pos-p.pos;float dist=length(d);float aim=dot(normalized(d),f);if(dist<bd&&aim>.96f&&l.lineClear(p.pos,e.pos)){best=&e;bd=dist;}}
  if(best)best->damage(bd<3.f?40:25);
}

