#include "renderer/Renderer.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "weapons/Shotgun.hpp"
#include "world/Level.hpp"
#include "ui/HudFace.hpp"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr float PI = 3.14159265f;
constexpr float FOV = PI / 3.0f;

std::uint32_t shade(std::uint32_t color, float amount) {
  amount = std::clamp(amount, 0.0f, 1.5f);
  const auto r = static_cast<unsigned>(std::min(255.0f, static_cast<float>((color >> 16u) & 255u) * amount));
  const auto g = static_cast<unsigned>(std::min(255.0f, static_cast<float>((color >> 8u) & 255u) * amount));
  const auto b = static_cast<unsigned>(std::min(255.0f, static_cast<float>(color & 255u) * amount));
  return 0xff000000u | (r << 16u) | (g << 8u) | b;
}
} // namespace

bool Renderer::init(SDL_Window* window) {
  window_ = window;
  context_ = SDL_GL_CreateContext(window);
  if (!context_) return false;
  SDL_GL_SetSwapInterval(1);
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels_.data());
  if (!shotgunSprites_.load("assets/sprites/weapons/rivet12_sheet_source.png", 6) &&
      !shotgunSprites_.load("../assets/sprites/weapons/rivet12_sheet_source.png", 6)) {
    SDL_Log("Weapon sprite load failed: %s", SDL_GetError());
  }
  if(!enemySprites_[0].load("assets/sprites/enemies/rusher_atlas.png",8)&&!enemySprites_[0].load("../assets/sprites/enemies/rusher_atlas.png",8))SDL_Log("Rusher sprite load failed: %s",SDL_GetError());
  if(!enemySprites_[1].load("assets/sprites/enemies/gunner_atlas.png",8)&&!enemySprites_[1].load("../assets/sprites/enemies/gunner_atlas.png",8))SDL_Log("Gunner sprite load failed: %s",SDL_GetError());
  if(!enemySprites_[2].load("assets/sprites/enemies/brute_atlas.png",8)&&!enemySprites_[2].load("../assets/sprites/enemies/brute_atlas.png",8))SDL_Log("Brute sprite load failed: %s",SDL_GetError());
  if(!faceSprites_.load("assets/sprites/ui/status_face_atlas.png",10)&&!faceSprites_.load("../assets/sprites/ui/status_face_atlas.png",10))SDL_Log("HUD face sprite load failed: %s",SDL_GetError());
  return true;
}

void Renderer::shutdown() {
  if (texture_) glDeleteTextures(1, &texture_);
  if (context_) SDL_GL_DestroyContext(context_);
  texture_ = 0;
  context_ = nullptr;
}

void Renderer::pixel(int x, int y, std::uint32_t color) {
  if (x >= 0 && y >= 0 && x < W && y < H) pixels_[y * W + x] = color;
}

void Renderer::rect(int x, int y, int w, int h, std::uint32_t color) {
  for (int yy = std::max(0, y); yy < std::min(H, y + h); ++yy)
    for (int xx = std::max(0, x); xx < std::min(W, x + w); ++xx) pixels_[yy * W + xx] = color;
}

void Renderer::text(int x, int y, const char* value, std::uint32_t color, int scale) {
  static const char* glyphs = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-";
  static const unsigned rows[] = {0,0x1e9f9,0x1e97a,0x0f842,0x1e8f9,0x1f87c,0x1f870,0x0f85b,0x118f1,0x1f210,0x0214a,0x11971,0x1087c,0x11dd1,0x11bd1,0x0e8ae,0x1e8f0,0x0e8b5,0x1e8f1,0x0f07a,0x1f210,0x118ae,0x118a4,0x11d77,0x11551,0x11544,0x1f124,0x0eaae,0x04644,0x1e1e1,0x1e1e1,0x12aaa,0x1f1e1,0x1f03e,0x1087f,0x1f0f1,0x1f0ff,0x1f111,0x00040,0x00400};
  for (; *value; ++value, x += 6 * scale) {
    const char* found = std::strchr(glyphs, *value);
    const unsigned bits = found ? rows[found - glyphs] : 0;
    for (int yy = 0; yy < 5; ++yy)
      for (int xx = 0; xx < 5; ++xx)
        if (bits & (1u << (yy * 5 + 4 - xx))) rect(x + xx * scale, y + yy * scale, scale, scale, color);
  }
}

void Renderer::drawSurfaces(const Level&, const Player& player, const Shotgun& gun) {
  const float visualAngle=player.angle+std::sin(time_*73.f)*player.screenShake()*.004f;
  const Vec2 direction{std::cos(visualAngle), std::sin(visualAngle)};
  const float planeLength = std::tan(FOV * 0.5f);
  const Vec2 plane{-direction.y * planeLength, direction.x * planeLength};
  const Vec2 rayLeft = direction - plane;
  const Vec2 rayRight = direction + plane;
  const int horizon = H / 2;

  std::fill(pixels_.begin(), pixels_.begin() + horizon * W, 0xff101724u);
  for (int y = horizon + 1; y < H; ++y) {
    const float rowDistance = (0.5f * H) / static_cast<float>(y - horizon);
    Vec2 floorPos = player.pos + rayLeft * rowDistance;
    const Vec2 step = (rayRight - rayLeft) * (rowDistance / W);
    const float fog = std::clamp(1.0f - rowDistance / 22.0f, 0.22f, 1.0f);
    const float muzzleLight=gun.flashStrength()*std::clamp(1.f-rowDistance/6.f,0.f,1.f)*.85f;
    for (int x = 0; x < W; ++x) {
      const int mapX = static_cast<int>(std::floor(floorPos.x));
      const int mapY = static_cast<int>(std::floor(floorPos.y));
      const float u = floorPos.x - std::floor(floorPos.x);
      const float v = floorPos.y - std::floor(floorPos.y);
      const Material floor = materials_.floor(mapX, mapY);
      const Material ceiling = materials_.ceiling(mapX, mapY);
      pixel(x, y, shade(atlas_.sample(floor.albedo, u, v), fog * floor.ambient+muzzleLight));
      pixel(x, H - y, shade(atlas_.sample(ceiling.albedo, u, v), fog * ceiling.ambient+muzzleLight*.8f));
      floorPos += step;
    }
  }
}

void Renderer::sprite(int centerX, int baseY, int size, std::uint32_t body, std::uint32_t eye, float animation) {
  const int top = baseY - size;
  for (int y = 0; y < size; ++y) {
    for (int x = -size / 2; x < size / 2; ++x) {
      const float nx = x / (size * 0.5f);
      const float ny = (y - size * 0.48f) / (size * 0.52f);
      if (nx * nx + ny * ny < 1.0f) pixel(centerX + x, top + y, shade(body, 0.72f + 0.28f * (1.0f - std::abs(nx))));
    }
  }
  const int bob = static_cast<int>(std::sin(animation * 7.0f) * size * 0.03f);
  rect(centerX - size / 5, top + size / 3 + bob, size / 9, size / 9, eye);
  rect(centerX + size / 10, top + size / 3 + bob, size / 9, size / 9, eye);
  rect(centerX - size / 3, top + size * 3 / 4, size * 2 / 3, std::max(1, size / 12), 0xff171523u);
}

void Renderer::worldSprite(float x, float y, int kind, const Player& player, const std::vector<float>& depth, float animation, float pain) {
  const float dx = x - player.pos.x;
  const float dy = y - player.pos.y;
  const float distance = std::sqrt(dx * dx + dy * dy);
  const float angle = wrapAngle(std::atan2(dy, dx) - player.angle);
  if (std::abs(angle) > FOV * 0.7f || distance < 0.15f) return;
  const int screenX = static_cast<int>(W * 0.5f + std::tan(angle) / std::tan(FOV * 0.5f) * W * 0.5f);
  const int size = static_cast<int>(H / (distance * 0.9f));
  const int base = H / 2 + size / 2;
  if (screenX < 0 || screenX >= W || distance >= depth[std::clamp(screenX, 0, W - 1)]) return;
  if (kind == 0) sprite(screenX, base, size, pain>0.f?0xffd9bec7u:0xff674d8fu, 0xff53f6d0u, animation);
  else {
    const std::uint32_t color = kind == 1 ? 0xff47c95eu : kind == 2 ? 0xffd6b744u : 0xff45cfe8u;
    const int h = std::max(3, size / 2);
    rect(screenX - h / 3, base - h, h * 2 / 3, h, color);
    rect(screenX - h / 2, base - h / 2, h, h / 3, shade(color, 0.6f));
  }
}

void Renderer::enemySprite(const Enemy& enemy,const Player& player,bool debug){
  const Vec2 delta=enemy.pos-player.pos;const float distance=length(delta);const float angle=wrapAngle(std::atan2(delta.y,delta.x)-player.angle);
  if(std::abs(angle)>FOV*.7f||distance<.15f)return;const int screenX=static_cast<int>(W*.5f+std::tan(angle)/std::tan(FOV*.5f)*W*.5f);
  if(screenX<0||screenX>=W||distance>=depth_[screenX]+.2f)return;const int size=static_cast<int>(H/(distance*.9f)*enemy.definition().visualScale);const int base=H/2+size/2;
  const int shadowW=std::max(3,size*2/5),shadowH=std::max(2,size/14);for(int y=0;y<shadowH;++y){const int inset=y*shadowW/(shadowH*3);rect(screenX-shadowW/2+inset,base-shadowH/2+y,shadowW-inset*2,1,0xff141419u);}
  const int row=(enemy.state==EnemyState::Dying||enemy.state==EnemyState::Dead)?6:enemy.animationFrame();const int column=enemy.directionFrame(player);
  const float brightness=enemy.painFlash>0?1.45f:1.f+enemy.muzzleFlash*.9f;
  enemySprites_[static_cast<std::size_t>(enemy.type)].drawCell(pixels_,W,H,column,row,7,screenX-size/2,base-size,size,size,brightness);
  if(enemy.muzzleFlash>0&&enemy.type!=EnemyType::Rusher){const int flashSize=std::max(3,size/8);rect(screenX+size/5,base-size*3/5,flashSize,flashSize,0xffffe476u);rect(screenX+size/5+2,base-size*3/5+2,flashSize/2,flashSize/2,0xffffffffu);}
  if(debug){char info[96];std::snprintf(info,sizeof(info),"%s %s H:%d D:%.1f F:%d",enemy.definition().name,enemy.stateName(),enemy.health,distance,enemy.animationFrame());text(std::max(2,screenX-static_cast<int>(std::strlen(info))*3),std::max(2,base-size-9),info,0xff53f6d0u);}
}

void Renderer::draw(const Level& level, const Player& player, const std::vector<Enemy>& enemies, const std::vector<EnemyProjectile>& projectiles, const Shotgun& gun, const HudFace& hudFace, bool won, bool debugEnemies) {
  time_ = static_cast<float>(SDL_GetTicksNS() / 1000000000.0);
  drawSurfaces(level, player, gun);
  const float visualAngle=player.angle+std::sin(time_*73.f)*player.screenShake()*.004f;
  const Vec2 direction{std::cos(visualAngle), std::sin(visualAngle)};
  const float planeLength = std::tan(FOV * 0.5f);
  const Vec2 plane{-direction.y * planeLength, direction.x * planeLength};

  for (int x = 0; x < W; ++x) {
    const float cameraX = 2.0f * x / W - 1.0f;
    const Vec2 ray{direction.x + plane.x * cameraX, direction.y + plane.y * cameraX};
    int mapX = static_cast<int>(player.pos.x);
    int mapY = static_cast<int>(player.pos.y);
    const float deltaX = ray.x == 0.0f ? 1e30f : std::abs(1.0f / ray.x);
    const float deltaY = ray.y == 0.0f ? 1e30f : std::abs(1.0f / ray.y);
    const int stepX = ray.x < 0.0f ? -1 : 1;
    const int stepY = ray.y < 0.0f ? -1 : 1;
    float sideX = (ray.x < 0.0f ? player.pos.x - mapX : mapX + 1.0f - player.pos.x) * deltaX;
    float sideY = (ray.y < 0.0f ? player.pos.y - mapY : mapY + 1.0f - player.pos.y) * deltaY;
    bool side = false;
    char hit = '#';
    for (int guard = 0; guard < 64; ++guard) {
      if (sideX < sideY) { sideX += deltaX; mapX += stepX; side = false; }
      else { sideY += deltaY; mapY += stepY; side = true; }
      hit = level.tile(mapX, mapY);
      if (hit == '#' || hit == 'D') break;
    }
    const float distance = std::max(0.001f, side ? sideY - deltaY : sideX - deltaX);
    depth_[x] = distance;
    const int wallHeight = std::min(H * 3, static_cast<int>(H / distance));
    const int top = H / 2 - wallHeight / 2;
    float wallX = side ? player.pos.x + distance * ray.x : player.pos.y + distance * ray.y;
    wallX -= std::floor(wallX);
    if ((!side && ray.x > 0.0f) || (side && ray.y < 0.0f)) wallX = 1.0f - wallX;
    const Material material = materials_.wall(hit, mapX, mapY, time_);
    const float fog = std::clamp(1.05f - distance / 18.0f, 0.2f, 1.0f);
    const float muzzleLight=gun.flashStrength()*std::clamp(1.f-distance/6.f,0.f,1.f)*.9f;
    const float light = fog * material.ambient * (side ? 0.78f : 1.0f) + material.emissive+muzzleLight;
    for (int y = std::max(0, top); y < std::min(H, top + wallHeight); ++y) {
      float v = static_cast<float>(y - top) / wallHeight;
      if (material.animated) v += time_ * 0.12f;
      pixel(x, y, shade(atlas_.sample(material.albedo, wallX, v), light));
    }
  }

  struct Visible { float distance;const Enemy* enemy;float x,y;int kind; };
  std::vector<Visible> sprites;
  sprites.reserve(enemies.size() + level.pickups.size());
  for (const auto& enemy : enemies)
    sprites.push_back({length(enemy.pos-player.pos),&enemy,0,0,0});
  for (const auto& pickup : level.pickups)
    if (pickup.active) sprites.push_back({length(pickup.pos-player.pos),nullptr,pickup.pos.x,pickup.pos.y,pickup.type==PickupType::Health?1:pickup.type==PickupType::Ammo?2:3});
  std::sort(sprites.begin(), sprites.end(), [](const auto& a, const auto& b) { return a.distance > b.distance; });
  for(const auto& item:sprites){if(item.enemy)enemySprite(*item.enemy,player,debugEnemies);else worldSprite(item.x,item.y,item.kind,player,depth_);}

  for(const auto& projectile:projectiles){const Vec2 delta=projectile.pos-player.pos;const float distance=length(delta);const float angle=wrapAngle(std::atan2(delta.y,delta.x)-visualAngle);if(std::abs(angle)>FOV*.65f||distance<.1f)continue;const int sx=static_cast<int>(W*.5f+std::tan(angle)/std::tan(FOV*.5f)*W*.5f);if(sx<0||sx>=W||distance>depth_[sx]+.2f)continue;const int base=H/2+static_cast<int>(H/(distance*.9f))*.18f;const int size=projectile.exploding?static_cast<int>(28+projectile.explosionTime*130.f):std::clamp(static_cast<int>(26.f/distance),5,22);rect(sx-size,base-size,size*2,size*2,projectile.exploding?0xff167f91u:0xff12636fu);rect(sx-size/2,base-size/2,size,size,0xff37e8e1u);rect(sx-size/4,base-size/4,std::max(2,size/2),std::max(2,size/2),0xffe8ffffu);if(!projectile.exploding){const Vec2 trail=normalized(projectile.velocity)*-.22f;for(int i=1;i<4;++i)rect(sx+static_cast<int>(trail.x*i*18),base+static_cast<int>(trail.y*i*8),std::max(2,size/3),std::max(2,size/3),0xff259aa2u);}}

  for(const auto& impact:gun.impacts()){
    const Vec2 delta=impact.pos-player.pos;const float distance=length(delta);const float angle=wrapAngle(std::atan2(delta.y,delta.x)-visualAngle);
    if(std::abs(angle)>FOV*.58f||distance<.15f)continue;const int sx=static_cast<int>(W*.5f+std::tan(angle)/std::tan(FOV*.5f)*W*.5f);
    if(sx<0||sx>=W||distance>depth_[sx]+.3f)continue;const int sy=H/2;const float life=1.f-impact.age/.38f;
    if(impact.kind==ImpactKind::Blood){for(int i=0;i<5;++i){const int ox=static_cast<int>(std::sin(impact.seed*9.f+i*2.1f)*18.f*(1.f-life));const int oy=static_cast<int>(i*5+impact.age*45.f);rect(sx+ox,sy-oy,4,4,i&1?0xffb62432u:0xff6f1724u);}}
    else {for(int i=0;i<4;++i){const int ox=static_cast<int>(std::sin(impact.seed+i)*24.f*impact.age);const int oy=static_cast<int>(std::cos(impact.seed+i*3.f)*18.f*impact.age);rect(sx+ox,sy+oy,3,3,i?0xffffa33au:0xffffeaa0u);}rect(sx-4,sy-4,8,8,0xff3c3534u);}
  }

  const float move=player.movementAmount();const float bobSpeed=player.running()?12.f:8.f;
  const float bobX=std::sin(time_*bobSpeed)*10.f*move;const float bobY=std::abs(std::cos(time_*bobSpeed))*(player.running()?8.f:5.f)*move+std::sin(time_*1.7f)*1.5f;
  const float recoilDown=gun.state()==WeaponState::Recoil?22.f*(1.f-std::min(1.f,gun.animation()/.12f)):gun.state()==WeaponState::Recover?12.f*(1.f-std::min(1.f,gun.animation()/.28f)):0.f;
  if(shotgunSprites_.valid())shotgunSprites_.draw(pixels_,W,H,gun.spriteFrame(),static_cast<int>(W/2-145+bobX),static_cast<int>(100+bobY+recoilDown),290,270,1.f);
  if(gun.state()==WeaponState::Recoil||gun.state()==WeaponState::Recover){const float t=gun.state()==WeaponState::Recoil?gun.animation():.12f+gun.animation();const int shellX=static_cast<int>(405+t*210.f);const int shellY=static_cast<int>(250-t*190.f+t*t*260.f);rect(shellX,shellY,9,4,0xffd6973au);rect(shellX+2,shellY,3,4,0xffffd267u);}

  rect(0, 326, W, 34, 0xff11131au); rect(0, 326, W, 3, 0xff53f6d0u);
  rect(386,314,52,46,0xff080a0fu);rect(388,316,48,44,0xff26313au);faceSprites_.drawCell(pixels_,W,H,hudFace.column(),hudFace.healthRow(player),4,390,316,44,44,1.f,false);
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "HEALTH:%03d", player.health); text(12, 340, buffer, 0xfff1e8c9u, 2);
  std::snprintf(buffer, sizeof(buffer), "AMMO:%02d", player.ammo); text(250, 340, buffer, 0xffffd65au, 2);
  text(460, 340, player.hasKey ? "KEY:CYAN" : "KEY:---", player.hasKey ? 0xff53f6d0u : 0xff777777u, 2);
  if (player.hurtFlash > 0.0f)
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) if (((x + y) & 5) == 0) pixels_[y * W + x] = 0xff8d2028u;
  if (won) { rect(70, 115, 500, 115, 0xee10131cu); text(144, 140, "SECTOR SECURED", 0xff53f6d0u, 4); text(160, 195, "PRESS ESC TO QUIT", 0xfff1e8c9u, 2); }
  present();
}

void Renderer::present() {
  static bool captured=false;static int captureFrames=0;
  if(!captured&&captureFrames++>12){if(const char* path=std::getenv("VOIDLOCK_CAPTURE_FRAME")){SDL_Surface* surface=SDL_CreateSurfaceFrom(W,H,SDL_PIXELFORMAT_ARGB8888,pixels_.data(),W*static_cast<int>(sizeof(std::uint32_t)));if(surface){SDL_SaveBMP(surface,path);SDL_DestroySurface(surface);captured=true;}}}
  int windowW = 0, windowH = 0;
  SDL_GetWindowSizeInPixels(window_, &windowW, &windowH);
  int viewportW = windowW;
  int viewportH = viewportW * H / W;
  if (viewportH > windowH) { viewportH = windowH; viewportW = viewportH * W / H; }
  const int integerScale = std::min(windowW / W, windowH / H);
  if (integerScale >= 1) { viewportW = W * integerScale; viewportH = H * integerScale; }
  const int viewportX = (windowW - viewportW) / 2;
  const int viewportY = (windowH - viewportH) / 2;

  glViewport(0, 0, windowW, windowH);
  glClearColor(0.015f, 0.018f, 0.025f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(viewportX, viewportY, viewportW, viewportH);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_BGRA, GL_UNSIGNED_BYTE, pixels_.data());
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 1); glVertex2f(-1, -1);
  glTexCoord2f(1, 1); glVertex2f(1, -1);
  glTexCoord2f(1, 0); glVertex2f(1, 1);
  glTexCoord2f(0, 0); glVertex2f(-1, 1);
  glEnd();
  SDL_GL_SwapWindow(window_);
}
