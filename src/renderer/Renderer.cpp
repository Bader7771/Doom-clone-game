#include "renderer/Renderer.hpp"
#include "entities/Enemy.hpp"
#include "entities/Player.hpp"
#include "weapons/Shotgun.hpp"
#include "world/Level.hpp"
#include <SDL3/SDL_opengl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace { constexpr float PI=3.14159265f,FOV=PI/3.f; unsigned shade(unsigned c,float s){unsigned r=(c>>16)&255,g=(c>>8)&255,b=c&255;r=(unsigned)(r*s);g=(unsigned)(g*s);b=(unsigned)(b*s);return 0xff000000|(r<<16)|(g<<8)|b;} }

bool Renderer::init(SDL_Window* w){
  window_=w;context_=SDL_GL_CreateContext(w);if(!context_)return false;
  SDL_GL_SetSwapInterval(1);glGenTextures(1,&texture_);glBindTexture(GL_TEXTURE_2D,texture_);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,W,H,0,GL_BGRA,GL_UNSIGNED_BYTE,pixels_.data());return true;
}
void Renderer::shutdown(){if(texture_)glDeleteTextures(1,&texture_);if(context_)SDL_GL_DestroyContext(context_);texture_=0;context_=nullptr;}
void Renderer::pixel(int x,int y,unsigned c){if(x>=0&&y>=0&&x<W&&y<H)pixels_[y*W+x]=c;}
void Renderer::rect(int x,int y,int w,int h,unsigned c){for(int yy=std::max(0,y);yy<std::min(H,y+h);++yy)for(int xx=std::max(0,x);xx<std::min(W,x+w);++xx)pixel(xx,yy,c);}

void Renderer::text(int x,int y,const char* s,unsigned c,int z){
  static const char* glyphs=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-";
  static const unsigned rows[]={0,0x1e9f9,0x1e97a,0x0f842,0x1e8f9,0x1f87c,0x1f870,0x0f85b,0x118f1,0x1f210,0x0214a,0x11971,0x1087c,0x11dd1,0x11bd1,0x0e8ae,0x1e8f0,0x0e8b5,0x1e8f1,0x0f07a,0x1f210,0x118ae,0x118a4,0x11d77,0x11551,0x11544,0x1f124,0x0eaae,0x04644,0x1e1e1,0x1e1e1,0x12aaa,0x1f1e1,0x1f03e,0x1087f,0x1f0f1,0x1f0ff,0x1f111,0x00040,0x00400};
  for(;*s;++s,x+=6*z){const char* p=std::strchr(glyphs,*s);unsigned bits=p?rows[p-glyphs]:0;for(int yy=0;yy<5;++yy)for(int xx=0;xx<5;++xx)if(bits&(1u<<(yy*5+4-xx)))rect(x+xx*z,y+yy*z,z,z,c);}
}

void Renderer::sprite(int cx,int base,int size,unsigned body,unsigned eye,bool dead,float anim){
  int top=base-size;for(int y=0;y<size;++y)for(int x=-size/2;x<size/2;++x){float nx=x/(size*.5f),ny=(y-size*.48f)/(size*.52f);if(nx*nx+ny*ny<1.f){unsigned c=shade(body,.75f+.25f*(1.f-std::abs(nx)));pixel(cx+x,top+y,c);}}
  if(dead){rect(cx-size/2,base-size/5,size,size/5,0xff452e46);return;}
  int bob=(int)(std::sin(anim*7)*size*.03f);rect(cx-size/5,top+size/3+bob,size/9,size/9,eye);rect(cx+size/10,top+size/3+bob,size/9,size/9,eye);
  rect(cx-size/3,top+size*3/4,size*2/3,std::max(1,size/12),0xff171523);
}
void Renderer::worldSprite(float x,float y,int kind,const Player& p,const std::vector<float>& z,float anim){
  float dx=x-p.pos.x,dy=y-p.pos.y,dist=std::sqrt(dx*dx+dy*dy);float da=wrapAngle(std::atan2(dy,dx)-p.angle);if(std::abs(da)>FOV*.7f||dist<.15f)return;
  int sx=(int)(W/2+std::tan(da)/(std::tan(FOV/2))*W/2),size=(int)(H/(dist*.9f));int base=H/2+size/2;
  if(sx<0||sx>=W||dist>=z[std::clamp(sx,0,W-1)])return;
  if(kind==0)sprite(sx,base,size,0xff674d8f,0xff53f6d0,false,anim);
  else {unsigned col=kind==1?0xff47c95e:kind==2?0xffd6b744:0xff45cfe8;int h=std::max(3,size/2);rect(sx-h/3,base-h,h*2/3,h,col);rect(sx-h/2,base-h/2,h,h/3,shade(col,.6f));}
}

void Renderer::draw(const Level& l,const Player& p,const std::vector<Enemy>& enemies,const Shotgun& gun,bool won){
  std::vector<float> depth(W,99.f);
  for(int y=0;y<H/2;++y)for(int x=0;x<W;++x)pixels_[y*W+x]=shade(0xff172036,.55f+(float)y/H*.35f);
  for(int y=H/2;y<H;++y)for(int x=0;x<W;++x){int q=((x/16)+(y/10))&1;pixels_[y*W+x]=q?0xff22242a:0xff292b31;}
  for(int x=0;x<W;++x){float ray=p.angle-FOV/2+FOV*(x+.5f)/W;float cs=std::cos(ray),sn=std::sin(ray),d=.02f;char hit='.';float hx=0,hy=0;for(;d<24;d+=.025f){hx=p.pos.x+cs*d;hy=p.pos.y+sn*d;hit=l.tile((int)hx,(int)hy);if(hit=='#'||hit=='D')break;}d*=std::cos(ray-p.angle);depth[x]=d;int wh=std::min(H*2,(int)(H/d));int y0=H/2-wh/2;float edge=std::min({std::fmod(hx,1.f),std::fmod(hy,1.f),1-std::fmod(hx,1.f),1-std::fmod(hy,1.f)});unsigned base=hit=='D'?0xff8d7137:0xff53616a;float light=std::clamp(1.05f-d/17.f,.25f,1.f);if(edge<.035f)light*=.62f;for(int y=std::max(0,y0);y<std::min(H,y0+wh);++y){float stripe=((y-y0)/(std::max(1,wh/8)))%2?1.f:.91f;pixel(x,y,shade(base,light*stripe));}}
  struct S{float d,x,y;int k;float a;};std::vector<S> ss;
  for(auto& e:enemies)if(e.state!=EnemyState::Dead)ss.push_back({length(e.pos-p.pos),e.pos.x,e.pos.y,0,e.anim});
  for(auto& q:l.pickups)if(q.active)ss.push_back({length(q.pos-p.pos),q.pos.x,q.pos.y,q.type==PickupType::Health?1:q.type==PickupType::Ammo?2:3,0});
  std::sort(ss.begin(),ss.end(),[](auto&a,auto&b){return a.d>b.d;});for(auto&s:ss)worldSprite(s.x,s.y,s.k,p,depth,s.a);
  int recoil=(int)(std::sin(std::min(gun.animation(),.7f)*PI/.7f)*18);rect(123,159+recoil,74,41,0xff2b2025);rect(137,150+recoil,46,35,0xff80664c);rect(153,139+recoil,14,30,0xff343942);rect(157,139+recoil,6,18,0xff15181c);if(gun.flash()>0){rect(148,126,24,13,0xffffd65a);rect(154,118,12,28,0xffff8338);}
  rect(0,181,W,19,0xff11131a);rect(0,181,W,2,0xff53f6d0);char buf[64];std::snprintf(buf,sizeof(buf),"HEALTH:%03d",p.health);text(6,188,buf,0xfff1e8c9);std::snprintf(buf,sizeof(buf),"AMMO:%02d",p.ammo);text(119,188,buf,0xffffd65a);text(224,188,p.hasKey?"KEY:CYAN":"KEY:---",p.hasKey?0xff53f6d0:0xff777777);
  if(p.hurtFlash>0)for(int y=0;y<H;++y)for(int x=0;x<W;++x)if(((x+y)&3)==0)pixels_[y*W+x]=0xff8d2028;
  if(won){rect(35,65,250,65,0xee10131c);text(72,78,"SECTOR SECURED",0xff53f6d0,2);text(80,108,"PRESS ESC TO QUIT",0xfff1e8c9);}
  glViewport(0,0,1,1);int ww,hh;SDL_GetWindowSizeInPixels(window_,&ww,&hh);glViewport(0,0,ww,hh);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);glBindTexture(GL_TEXTURE_2D,texture_);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_BGRA,GL_UNSIGNED_BYTE,pixels_.data());glEnable(GL_TEXTURE_2D);glBegin(GL_QUADS);glTexCoord2f(0,1);glVertex2f(-1,-1);glTexCoord2f(1,1);glVertex2f(1,-1);glTexCoord2f(1,0);glVertex2f(1,1);glTexCoord2f(0,0);glVertex2f(-1,1);glEnd();SDL_GL_SwapWindow(window_);
}

