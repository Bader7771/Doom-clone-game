#include "input/Input.hpp"
#include <algorithm>

bool Input::update() {
    std::copy(std::begin(current_), std::end(current_), std::begin(previous_));
    mouseDx_ = 0;
    fire_ = false;
    interact_ = false;
    fullscreenToggle_ = false;
    flashlightToggle_ = false;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT)
            return false;
        if (e.type == SDL_EVENT_MOUSE_MOTION)
            mouseDx_ += e.motion.xrel;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
            fire_ = true;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.scancode == SDL_SCANCODE_E)
            interact_ = true;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat &&
            e.key.scancode == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT))
            fullscreenToggle_ = true;
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.scancode == SDL_SCANCODE_F)
            flashlightToggle_ = true;
    }
    keys_ = SDL_GetKeyboardState(nullptr);
    std::copy(keys_, keys_ + SDL_SCANCODE_COUNT, std::begin(current_));
    if (current_[SDL_SCANCODE_ESCAPE])
        return false;
    if (current_[SDL_SCANCODE_SPACE])
        fire_ = true;
    return true;
}
bool Input::down(SDL_Scancode k) const {
    return current_[k];
}
bool Input::pressed(SDL_Scancode k) const {
    return current_[k] && !previous_[k];
}
