#pragma once
#include <cmath>

struct Vec2 {
    float x{}, y{};
    Vec2 operator+(Vec2 b) const {
        return {x + b.x, y + b.y};
    }
    Vec2 operator-(Vec2 b) const {
        return {x - b.x, y - b.y};
    }
    Vec2 operator*(float s) const {
        return {x * s, y * s};
    }
    Vec2& operator+=(Vec2 b) {
        x += b.x;
        y += b.y;
        return *this;
    }
};
inline float length(Vec2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}
inline Vec2 normalized(Vec2 v) {
    float l = length(v);
    return l > 0.0001f ? v * (1.f / l) : Vec2{};
}
inline float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}
inline float wrapAngle(float a) {
    constexpr float pi = 3.14159265f;
    while (a > pi)
        a -= 2 * pi;
    while (a < -pi)
        a += 2 * pi;
    return a;
}
