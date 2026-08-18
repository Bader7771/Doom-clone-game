# HUD portrait art

`status_face_atlas.png` is an original VOIDLOCK protagonist portrait atlas. It is not derived from a commercial game character or HUD asset.

Columns contain forward variations, blink, left/right looks, shotgun aggression, directional pain, and a restrained kill reaction. Rows represent healthy, scratched, heavily injured, and critical-health appearances; the last cell is the dedicated death portrait.

The portrait is rendered at 44×44 pixels inside the existing HUD using nearest-neighbor sprite sampling. Its flat green source background is removed by the dependency-free sprite loader.
