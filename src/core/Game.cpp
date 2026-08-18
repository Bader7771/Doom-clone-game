#include "core/Game.hpp"
#include <algorithm>
#include <cmath>

bool Game::init(){
  if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))return false;
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  window_=SDL_CreateWindow("VOIDLOCK",960,600,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);if(!window_)return false;
  if(!renderer_.init(window_))return false;SDL_SetWindowRelativeMouseMode(window_,true);audio_.init();for(auto p:level_.enemySpawns)enemies_.emplace_back(p);return true;
}
Game::~Game(){renderer_.shutdown();if(window_)SDL_DestroyWindow(window_);SDL_Quit();}
void Game::update(float dt){
  if(won_||player_.health<=0)return;player_.update(dt,input_,level_);Vec2 f{std::cos(player_.angle),std::sin(player_.angle)};
  if(input_.interact())level_.tryOpenDoor(player_.pos,f,player_.hasKey);
  shotgun_.update(dt,input_.fire(),player_,level_,enemies_,audio_);for(auto& e:enemies_)e.update(dt,player_,level_);
  for(auto& q:level_.pickups)if(q.active&&length(q.pos-player_.pos)<.5f){if(q.type==PickupType::Health&&player_.health<100){player_.health=std::min(100,player_.health+35);q.active=false;}else if(q.type==PickupType::Ammo){player_.ammo+=8;q.active=false;}else if(q.type==PickupType::Key){player_.hasKey=true;q.active=false;}if(!q.active)audio_.playPickup();}
  bool finalRoomClear=true;
  for(const auto& e:enemies_)if(e.state!=EnemyState::Dead&&e.pos.x>15.f)finalRoomClear=false;
  if(level_.atExit(player_.pos)&&finalRoomClear)won_=true;
}
int Game::run(){Uint64 last=SDL_GetTicksNS();while(input_.update()){Uint64 now=SDL_GetTicksNS();float dt=std::min(.05f,(now-last)/1000000000.f);last=now;update(dt);renderer_.draw(level_,player_,enemies_,shotgun_,won_);if(player_.health<=0){player_.health=100;player_.pos={2.5f,2.5f};}}return 0;}
