#define SDL_MAIN_HANDLED
#include "core/Game.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
int main(){SDL_SetAppMetadata("VOIDLOCK","0.1.0","dev.voidlock.game");Game game;if(!game.init()){std::fprintf(stderr,"VOIDLOCK failed: %s\n",SDL_GetError());return 1;}return game.run();}

