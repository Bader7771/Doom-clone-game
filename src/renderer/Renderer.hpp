#pragma once
#include <SDL3/SDL.h>
#include <vector>
class Level; class Player; class Enemy; class Shotgun;

class Renderer {
public:
  static constexpr int W=320,H=200;
  bool init(SDL_Window* window);
  void draw(const Level&,const Player&,const std::vector<Enemy>&,const Shotgun&,bool won);
  void shutdown();
private:
  void pixel(int x,int y,unsigned color);
  void rect(int x,int y,int w,int h,unsigned color);
  void text(int x,int y,const char* s,unsigned color,int scale=1);
  void sprite(int cx,int base,int size,unsigned body,unsigned eye,bool dead,float anim);
  void worldSprite(float x,float y,int kind,const Player&,const std::vector<float>& depth,float anim=0);
  SDL_Window* window_{}; SDL_GLContext context_{}; unsigned texture_{};
  std::vector<unsigned> pixels_ = std::vector<unsigned>(W * H);
};
