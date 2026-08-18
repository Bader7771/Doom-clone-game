#include "renderer/TextureAtlas.hpp"

#include <algorithm>
#include <cmath>

namespace {
std::uint32_t rgb(int r, int g, int b) {
    return 0xff000000u | (static_cast<std::uint32_t>(r) << 16u) |
           (static_cast<std::uint32_t>(g) << 8u) | static_cast<std::uint32_t>(b);
}

int noise(int x, int y, int seed) {
    std::uint32_t n = static_cast<std::uint32_t>(x * 1973 + y * 9277 + seed * 26699) | 1u;
    n = (n << 13u) ^ n;
    return static_cast<int>((n * (n * n * 15731u + 789221u) + 1376312589u) >> 27u) & 31;
}
} // namespace

TextureAtlas::TextureAtlas()
    : pixels_(static_cast<std::size_t>(TextureId::Count) * TileSize * TileSize) {
    build();
}

void TextureAtlas::set(TextureId texture, int x, int y, std::uint32_t color) {
    const auto tile = static_cast<std::size_t>(texture);
    pixels_[tile * TileSize * TileSize + y * TileSize + x] = color;
}

std::uint32_t TextureAtlas::sample(TextureId texture, float u, float v) const {
    u -= std::floor(u);
    v -= std::floor(v);
    const int x = std::clamp(static_cast<int>(u * TileSize), 0, TileSize - 1);
    const int y = std::clamp(static_cast<int>(v * TileSize), 0, TileSize - 1);
    const auto tile = static_cast<std::size_t>(texture);
    return pixels_[tile * TileSize * TileSize + y * TileSize + x];
}

void TextureAtlas::build() {
    for (int y = 0; y < TileSize; ++y) {
        for (int x = 0; x < TileSize; ++x) {
            const int grit = noise(x, y, 3) - 15;
            const bool seam = y % 16 == 0 || x % 16 == 0;
            set(TextureId::Concrete,
                x,
                y,
                seam ? rgb(43, 50, 54) : rgb(78 + grit, 88 + grit, 91 + grit));

            const bool crack = (x > 8 && x < 11 && y > 5 && y < 20) ||
                               (y == 19 && x > 9 && x < 23) || (x == 22 && y > 18 && y < 28);
            set(TextureId::DamagedConcrete,
                x,
                y,
                crack ? rgb(27, 29, 31) : rgb(68 + grit, 73 + grit, 72 + grit));

            const bool metalSeam = x % 16 == 0 || y % 16 == 0;
            const bool rivet = (x % 16 == 3 || x % 16 == 13) && (y % 16 == 3 || y % 16 == 13);
            set(TextureId::IndustrialMetal,
                x,
                y,
                rivet       ? rgb(142, 151, 150)
                : metalSeam ? rgb(35, 45, 50)
                            : rgb(72 + grit / 2, 83 + grit / 2, 87 + grit / 2));

            const bool rust = noise(x / 2, y / 2, 8) > 20;
            set(TextureId::RustedMetal,
                x,
                y,
                rust ? rgb(112 + grit, 55 + grit / 2, 31)
                     : rgb(65 + grit / 2, 69 + grit / 2, 66 + grit / 2));

            const bool doorEdge = x < 3 || x > 28 || x % 8 == 0;
            const bool doorLight = y > 5 && y < 9 && x > 5 && x < 27;
            set(TextureId::SecurityDoor,
                x,
                y,
                doorLight  ? rgb(45, 225, 211)
                : doorEdge ? rgb(35, 42, 44)
                           : rgb(115 + grit / 3, 93 + grit / 3, 48));

            const bool stripe = ((x + y) / 6) % 2 == 0;
            set(TextureId::WarningPanel, x, y, stripe ? rgb(198, 145, 35) : rgb(34, 31, 29));

            const bool screen = x > 4 && x < 27 && y > 5 && y < 22;
            const bool scan = y % 4 == 0;
            set(TextureId::ComputerPanel,
                x,
                y,
                screen ? (scan ? rgb(32, 116, 109) : rgb(38, 207, 177)) : rgb(34, 42, 49));

            const bool plateSeam = x % 16 == 0 || y % 16 == 0;
            set(TextureId::FloorPlate,
                x,
                y,
                plateSeam ? rgb(25, 29, 32) : rgb(57 + grit / 3, 62 + grit / 3, 63 + grit / 3));
            const bool stain = noise(x / 3, y / 3, 14) > 23;
            set(TextureId::DirtyFloor,
                x,
                y,
                stain ? rgb(48, 43, 34) : rgb(67 + grit / 3, 65 + grit / 3, 57 + grit / 3));
            set(TextureId::HazardFloor, x, y, stripe ? rgb(151, 105, 25) : rgb(34, 36, 36));

            const bool ceilingSeam = x % 16 == 0 || y % 16 == 0;
            set(TextureId::CeilingPanel,
                x,
                y,
                ceilingSeam ? rgb(30, 37, 42) : rgb(52 + grit / 4, 62 + grit / 4, 68 + grit / 4));
            const bool pipe = (x > 5 && x < 10) || (x > 21 && x < 26);
            const bool pipeEdge = x == 6 || x == 9 || x == 22 || x == 25;
            set(TextureId::CeilingPipes,
                x,
                y,
                pipeEdge ? rgb(27, 31, 34)
                : pipe   ? rgb(93, 72, 54)
                         : rgb(39 + grit / 4, 44 + grit / 4, 47 + grit / 4));
        }
    }
}
