#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "world/Level.hpp"
#include <algorithm>

void Enemy::damage(int n){if(state==EnemyState::Dead)return;health-=n;if(health<=0)state=EnemyState::Dead;else state=EnemyState::Chase;}
void Enemy::update(float dt,Player& p,const Level& l){
  anim+=dt;attackCooldown=std::max(0.f,attackCooldown-dt);if(state==EnemyState::Dead)return;
  float d=length(p.pos-pos);if(state==EnemyState::Idle&&d<7.f&&l.lineClear(pos,p.pos))state=EnemyState::Chase;
  if(state==EnemyState::Chase){if(d<.85f)state=EnemyState::Attack;else{Vec2 step=normalized(p.pos-pos)*dt*.75f;Vec2 np=pos+step;if(!l.solid(np.x,np.y))pos=np;}}
  if(state==EnemyState::Attack){if(d>1.1f)state=EnemyState::Chase;else if(attackCooldown<=0){p.hurt(8);attackCooldown=.9f;}}
}

