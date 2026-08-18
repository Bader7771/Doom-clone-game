#include "audio/Audio.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <vector>
bool Audio::init(){ready_=true;return true;}
void Audio::playShot(){/* Hook for a future SDL_AudioStream or assets/sounds/shot.wav. */}
void Audio::playPickup(){/* Placeholder support intentionally silent when no asset exists. */}
