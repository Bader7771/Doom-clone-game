#pragma once
#include <SDL3/SDL.h>

class Input {
  public:
    bool update();
    bool down(SDL_Scancode key) const;
    bool pressed(SDL_Scancode key) const;
    float mouseDx() const {
        return mouseDx_;
    }
    bool fire() const {
        return fire_;
    }
    bool interact() const {
        return interact_;
    }
    bool fullscreenToggle() const {
        return fullscreenToggle_;
    }
    bool flashlightToggle() const {
        return flashlightToggle_;
    }

  private:
    const bool* keys_{};
    bool previous_[SDL_SCANCODE_COUNT]{};
    bool current_[SDL_SCANCODE_COUNT]{};
    float mouseDx_{};
    bool fire_{}, interact_{}, fullscreenToggle_{}, flashlightToggle_{};
};
