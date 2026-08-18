#include "entities/Player.hpp"
#include "input/Input.hpp"
#include "world/Level.hpp"
#include <algorithm>
#include <cmath>

void Player::move(Vec2 d,const Level& l){constexpr float r=.22f;if(!l.solid(pos.x+d.x+(d.x>0?r:-r),pos.y))pos.x+=d.x;if(!l.solid(pos.x,pos.y+d.y+(d.y>0?r:-r)))pos.y+=d.y;}
void Player::update(float dt,const Input& in,const Level& l){
  angle=wrapAngle(angle+in.mouseDx()*.0025f); Vec2 f{std::cos(angle),std::sin(angle)}, r{-f.y,f.x}, v{};
  if(in.down(SDL_SCANCODE_W))v+=f;if(in.down(SDL_SCANCODE_S))v+=f*-1;if(in.down(SDL_SCANCODE_D))v+=r;if(in.down(SDL_SCANCODE_A))v+=r*-1;
  float speed=in.down(SDL_SCANCODE_LSHIFT)?4.5f:2.8f;if(length(v)>0)move(normalized(v)*speed*dt,l);
  hurtFlash=std::max(0.f,hurtFlash-dt);
}
void Player::hurt(int n){health=std::max(0,health-n);hurtFlash=.22f;}

