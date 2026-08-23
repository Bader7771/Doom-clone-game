#include "world/Level.hpp"
#include <cmath>

Level::Level() {
    grid_ = {"#########################",
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
             "#########################"};
    h_ = static_cast<int>(grid_.size());
    w_ = static_cast<int>(grid_[0].size());

    // -------------------------------------------------------------------------
    // Pickup placement — survival-horror resource tension design:
    //   Zone A (spawn):   starting shells only — no health nearby at start
    //   Zone B (mid):     first health kit after dark corridor encounter
    //   Zone C (far):     batteries scattered, ammo refill before final room
    //   Zone D (final):   keycard in open, health behind last group of zombies
    // -------------------------------------------------------------------------
    pickups = {
        // Zone A — starting supplies near spawn
        {{4.5f, 2.5f}, PickupType::Ammo}, // shells in first room

        // Zone A→B corridor — first battery in dark hallway
        {{3.5f, 5.5f}, PickupType::FlashlightBattery}, // dark side corridor

        // Zone B — mid-map rewards for exploring lower area
        {{3.5f, 10.5f}, PickupType::Health}, // medical kit behind pillar
        {{6.5f, 12.5f}, PickupType::Ammo},   // shells in lower storage
        {{9.5f, 8.5f}, PickupType::Ammo},    // shells in open passage

        // Zone B→C — battery reward for going off the direct path
        {{14.5f, 2.5f}, PickupType::FlashlightBattery}, // upper wing alcove

        // Zone C — far-wing resources before the locked door
        {{18.5f, 5.5f}, PickupType::Ammo}, // shells in far wing

        // Zone D — keycard and final room resources
        {{12.5f, 10.5f}, PickupType::Key},               // keycard in junction
        {{16.5f, 11.5f}, PickupType::FlashlightBattery}, // battery near exit
        {{20.5f, 2.5f}, PickupType::Health},             // health behind far zombie
        {{22.5f, 11.5f}, PickupType::Ammo},              // final shells near exit
    };

    // -------------------------------------------------------------------------
    // Zombie placement — spread across zones, never at spawn, escalating danger:
    //   Zone A: no zombies — player can orient themselves safely
    //   Zone A→B: first zombie encounter in corridor (solo, manageable)
    //   Zone B: two zombies in lower area for mid-game tension
    //   Zone C: one zombie in far wing
    //   Zone D: three zombies in final room — boss-like challenge
    // -------------------------------------------------------------------------
    enemySpawns = {{{10.5f, 2.5f}, EnemyType::Rusher},
                   {{8.5f, 9.5f},  EnemyType::Rusher},
                   {{12.5f, 12.f}, EnemyType::Gunner},
                   {{19.5f, 3.5f}, EnemyType::Brute},
                   {{18.5f, 10.5f},EnemyType::Rusher},
                   {{21.f, 9.5f},  EnemyType::Gunner},
                   {{21.5f, 12.5f},EnemyType::Brute}};
}

char Level::tile(int x, int y) const {
    return x < 0 || y < 0 || x >= w_ || y >= h_ ? '#' : grid_[y][x];
}
bool Level::solid(float x, float y) const {
    char c = tile((int)x, (int)y);
    return c == '#' || c == 'D';
}
bool Level::tryOpenDoor(Vec2 p, Vec2 f, bool key) {
    int x = (int)(p.x + f.x * .9f), y = (int)(p.y + f.y * .9f);
    if (tile(x, y) == 'D' && key) {
        grid_[y][x] = '.';
        return true;
    }
    return false;
}
bool Level::lineClear(Vec2 a, Vec2 b) const {
    Vec2 d = b - a;
    float dist = length(d);
    d = normalized(d);
    for (float t = .1f; t < dist; t += .1f)
        if (solid(a.x + d.x * t, a.y + d.y * t))
            return false;
    return true;
}
bool Level::atExit(Vec2 p) const {
    return tile((int)p.x, (int)p.y) == 'X';
}
