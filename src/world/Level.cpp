#include "world/Level.hpp"
#include <cmath>

Level::Level() {
  grid_={
    "#########################",
    "#....#........#.........#",
    "#....#........#.........#",
    "#....#........#.........#",
    "#....####.#####.........#",
    "#.......................#",
    "#....####.######D########",
    "#....#.........#........#",
    "######.........#........#",
    "#..............#........#",
    "#..............#........#",
    "#..............#........#",
    "#..............#........#",
    "#..............#......X.#",
    "#########################"
  };
  h_=static_cast<int>(grid_.size()); w_=static_cast<int>(grid_[0].size());
  pickups={{{7.5f,2.5f},PickupType::Ammo},{{3.5f,10.5f},PickupType::Health},{{12.5f,10.5f},PickupType::Key},{{18.5f,9.5f},PickupType::Ammo}};
  enemySpawns={{{10.5f,2.5f},EnemyType::Rusher},{{8.5f,9.5f},EnemyType::Rusher},{{12.5f,12.f},EnemyType::Gunner},{{19.5f,3.5f},EnemyType::Brute},{{18.5f,10.5f},EnemyType::Rusher},{{21.f,9.5f},EnemyType::Gunner},{{21.5f,12.5f},EnemyType::Brute}};
}
char Level::tile(int x,int y) const { return x<0||y<0||x>=w_||y>=h_?'#':grid_[y][x]; }
bool Level::solid(float x,float y) const { char c=tile((int)x,(int)y); return c=='#'||c=='D'; }
bool Level::tryOpenDoor(Vec2 p,Vec2 f,bool key) {
  int x=(int)(p.x+f.x*.9f), y=(int)(p.y+f.y*.9f);
  if(tile(x,y)=='D'&&key){grid_[y][x]='.';return true;} return false;
}
bool Level::lineClear(Vec2 a,Vec2 b) const {
  Vec2 d=b-a; float dist=length(d); d=normalized(d);
  for(float t=.1f;t<dist;t+=.1f) if(solid(a.x+d.x*t,a.y+d.y*t)) return false;
  return true;
}
bool Level::atExit(Vec2 p) const { return tile((int)p.x,(int)p.y)=='X'; }
