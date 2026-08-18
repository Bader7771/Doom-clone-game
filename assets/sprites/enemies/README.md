# Enemy art

These atlases are original assets created for VOIDLOCK and are not derived from commercial game sprites.

- `rusher_atlas.png`: thin corrupted pursuit creature, 8 directions and 7 animation rows.
- `gunner_atlas.png`: medium industrial burst-fire combatant, 8 directions and 7 animation rows.
- `brute_atlas.png`: heavy energy-core siege creature, 8 directions and 7 animation rows. Its flat green source background is removed by the dependency-free sprite loader at load time.

Atlas columns are front, front-left, left, back-left, back, back-right, right, and front-right. Rows contain idle, alert, two movement poses, attack anticipation, attack impact, and pain/death artwork. Enemy sprites are sampled into the 640×360 framebuffer with nearest-neighbor filtering.
