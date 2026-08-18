#include "entities/Player.hpp"
#include "input/Input.hpp"
#include "world/Level.hpp"
#include <algorithm>
#include <cmath>

void Player::move(Vec2 d,const Level& l){constexpr float r=.22f;if(!l.solid(pos.x+d.x+(d.x>0?r:-r),pos.y))pos.x+=d.x;if(!l.solid(pos.x,pos.y+d.y+(d.y>0?r:-r)))pos.y+=d.y;}
void Player::update(float dt,const Input& in,const Level& l){
  angle=wrapAngle(angle+in.mouseDx()*.0025f); Vec2 f{std::cos(angle),std::sin(angle)}, r{-f.y,f.x}, v{};
  if(in.down(SDL_SCANCODE_W))v+=f;if(in.down(SDL_SCANCODE_S))v+=f*-1;if(in.down(SDL_SCANCODE_D))v+=r;if(in.down(SDL_SCANCODE_A))v+=r*-1;
  lateralMovement_=(in.down(SDL_SCANCODE_D)?1.f:0.f)-(in.down(SDL_SCANCODE_A)?1.f:0.f);
  running_=in.down(SDL_SCANCODE_LSHIFT);float speed=running_?4.5f:2.8f;const bool moving=length(v)>0;if(moving)move(normalized(v)*speed*dt,l);
  movementAmount_+=((moving?1.f:0.f)-movementAmount_)*std::min(1.f,dt*12.f);
  viewKick_+=(-viewKick_)*std::min(1.f,dt*15.f);screenShake_=std::max(0.f,screenShake_-dt*7.f);
  hurtFlash=std::max(0.f,hurtFlash-dt);
}
void Player::hurt(int n,Vec2 source){health=std::max(0,health-n);hurtFlash=.22f;lastDamage_=n;++damageSerial_;const Vec2 right{-std::sin(angle),std::cos(angle)};const Vec2 incoming=normalized(source-pos);damageSide_=dot(incoming,right);}
void Player::addWeaponKick(float recoil,float shake){viewKick_=std::min(.09f,viewKick_+recoil);screenShake_=std::min(1.f,screenShake_+shake);}
