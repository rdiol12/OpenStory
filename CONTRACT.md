# OpenStory AI Equipment — Authoring Contract

The canonical spec for creating AI-generated equipment for this client. One pipeline per
slot class. Supersedes any older instructions.

## Core principles

- Everything ships inside the NX files (`Character.nx`, `String.nx`) served by the
  launcher. No loose files, no per-frame hand art.
- Every item starts as a **clone of a vanilla donor `.img`**: all stances, frames,
  origins, map anchors, z values and delays copied **unchanged**. Geometry is never
  authored — AI supplies only surface (materials) and shape (single authored images).
- Every item also needs: `info` stats/reqs, a String.wz name+desc entry, and the server's
  own wz updated so `!item <id>` works.
- **Never use:** `aiFit`, `aiShell` on hats or weapons, `replaceHead`, `aiShellFullHead`,
  `aiShellSeat`. Removed/ignored by the client.
- **Never author icons** — the client synthesizes them (see Icons).

## 1. Materials — `info/aiSkin` (armor, capes, gloves, shoes, earrings, weapons)

One flat, **seamless**, fully **opaque** material swatch (256–512px). Not a garment
picture — no shape, outline, background, or lighting gradient. The client retextures
every donor frame from it at load: pixels map into body-space (pinned to each part's
anchor), sample the material with wrap-around, and are shaded by the donor pixel's
luminance — every pose keeps its hand-drawn folds, shadows and outline, and alignment
can never break.

Optional knobs (numbers under `info/`, tunable by republish alone):

| Node | Meaning | Default |
|---|---|---|
| `aiSkinScale` | sprite px per material width (smaller = bigger features) | 24 |
| `aiSkinShadeMid` | donor luminance that leaves color unchanged | 0.74 |
| `aiSkinShadeStrength` | 0 flat .. 1 full donor shading | 0.40 |
| `aiSkinGloss` | highlight response: 0.5 matte .. 2.0 glassy | 1.0 |
| `aiSkinBands` | cel-shading steps (1 = smooth) | 4 |
| `aiSkinTrim` | `0xRRGGBB` int — recolors the 1px silhouette boundary | off |
| `aiSkinDecal` | bitmap emblem at 1x sprite scale over the material | off |
| `aiSkinDecalPos` | vector: decal center in body-space rel. the part anchor (shell items: rel. the collar origin) | (0,0) |
| `aiSkinArm` | second material used only for sleeve parts | off |

## 2. Custom silhouettes — `info/aiShell` (body armor and capes ONLY)

Authored silhouette images replace the donor body part — the garment's shape is whatever
is drawn. Author as **grayscale-shaded forms** when combined with `aiSkin` (material
supplies color, shading comes from the view's own luminance), or fully colored standalone.

Views (each a bitmap, or a container of numbered bitmaps `0,1,2` for animation riding the
body's stance timing — keep silhouettes near-identical between animation frames):

| View | Used for | If missing |
|---|---|---|
| `upright` (**required**) | stand/walk/alert/fly/heal + fallback | — |
| `attack` | all swing/stab/shoot stances | upright (auto spine-lean applies either way) |
| `jump` | airborne | upright squashed 18% |
| `prone` | lying (author head to the LEFT) | upright rotated to lying |
| `back` | ladder/rope | upright |
| `sit` | sitting | upright |
| `<family>Behind` | back-piece drawn behind the body (open coats, wings) | none |

Rules: 1x sprite scale, crisp pixel art, clean alpha, 1px outline. Each view's **origin
node = the collar point** (prone: at the head end, left). The client pins it to the
body's neck per frame and additionally **leans the shell with the body's spine during
attacks** automatically. Sleeves stay donor-derived (`aiSkin`/`aiSkinArm`). Reference:
vanilla longcoat body ≈ 17×20 px; shells may be bigger (long robe ≈ 24×34), stay within
~48×48.

## 3. Hats — the mannequin pipeline

Full silhouette freedom; placement is baked into generation.

Templates: `mannequin_front.png` / `mannequin_back.png` (exported by the client to
`wz/Custom/HatTemplate/`) — the real in-game head+hair at 8× on flat green.
Constants: scale = 8, brow pixel in the 8× image = **(224, 192)**.

Per hat, per view (front; back for climbing):

1. Image-edit the mannequin: *"Paint \<design\> worn on this character's head. Keep the
   head, hair and green background EXACTLY unchanged — only add the hat."*
2. Extract by diffing vs the template: changed pixel (max channel delta > 24) = hat
   (keep generated color); unchanged = transparent. Drop isolated pixels (<3 changed
   neighbors, 8-connected).
3. Downscale the hat region /8 (area-average; a 1× pixel is opaque if ≥40% of its 8×8
   block changed). Crop to the opaque bbox.
4. `origin = ( round((224 - bbox_x_8x)/8), round((192 - bbox_y_8x)/8) )`
5. Embed as **standard cap data** in a donor clone: every stance frame = the front bitmap
   with that origin and `map/brow = (0,0)`; ladder/rope + `backDefault` = the back bitmap.
6. `vslot` by design: full-cover codes (with `Ay`/`As`) for helms/masks that enclose the
   head (hides hair); `CpH1H5` for sit-on-top hats. Never bare `Cp`.

Alternative for donor-shaped restyles: repaint the donor cap's bitmaps in place (img2img
over each sprite at 8×, downscale, clamp to the donor's alpha), geometry untouched.

## 4. Weapons — the procedural one-image system

A procedural weapon `.img` contains ONLY `info` + `default/weapon` — no stance groups:

```
default/weapon    one 96x96 canonical bitmap, blade pointing straight UP,
                  handle centered at the bottom
  origin (0,0), z "weapon"
  map/grip (48,80)   fixed constant — where the hand holds it
  map/tip  (48,Yt)   blade tip, top-center of the art
```

The client anchors the grip to the hand every frame and rotates by per-type motion
profiles (type from the id prefix). Frames whose arm is fused into the body sprite use
the angle **measured from the type's vanilla base weapon** at that exact frame.
Optional `default/weapon/effect` = blade glow animation riding the transform.
**Materials:** add `info/aiSkin` and the canonical bitmap is retextured.
**`info` must be complete:** `attack` class (1=1H swing set, 2=spear, 3=bow, 4=crossbow,
5=2H, 6=wand/staff, 7=knuckle, 9=gun), `attackSpeed`, `afterImage` (`swordOL`/`swordTL`/…),
`sfx` (`swordL`/…), `stand`, `walk`, `islot`/`vslot` `"Wp"`, stats/reqs. (The client has
type-derived fallbacks for attack class, sfx, afterimage and attackSpeed, but authored
values always win.)

## 5. Auras — `info/effect` (any equip)

| Node | Meaning |
|---|---|
| `effect` | inline animation subtree: `effect/0/` = back-layer frames (bitmaps + origin + delay, authored CENTERED on the origin), `effect/1/` = optional front layer. Or a name string resolving to `CharEff.img`. 4–8 frames, ~120ms delays. |
| `effectPivot` | 0 center (default), 1 head, 2 feet |
| `effectBlend` | 1 = additive glow (real light accumulation — fire/magic; author on transparent black) |
| `effectTint` | 1 = author white/gray; the client tints with the item's material accent |
| `effectTintColor` | `0xRRGGBB` explicit tint (overrides `effectTint`) |
| `effectScale` / `effectOpacity` | size multiplier / 0..1 intensity |
| `effectShow` | 1 = hidden while climbing, 2 = idle-only |
| `effectFlip` | 1 = mirrors with facing |
| `effectDrag` | motion-trail multiplier (default 1; 0 = pinned) |
| `effectPrio` | int; if >3 aura items are worn, highest priorities win |

Recommended default: `effectBlend=1, effectTint=1, effectShow=1`.

## 6. Icons — automatic

- Material items: donor icon auto-retextured.
- Shell items: icon auto-rendered from the `aiShell` upright view.
- Procedural weapons: icon auto-rendered from the canonical blade image.

## 7. Verification — never publish unseen

Item ids in `wz/Custom/preview.txt` → the client dumps at startup to
`wz/Custom/Preview/<id>/`:

- every synthesized part frame and shell view (retextured)
- aura frames with the accent tint applied
- hats: `onhead.upright.png` / `onhead.back.png` — composited on the real head+hair
- armor/capes/hats: `onbody.<pose>.png` — **the item on the complete character**
  (body + head + hair) in stand, walk, swing, prone, jump and ladder poses, using the
  exact in-game anchor and lean math

What the composites show **is** what the game renders. Only publish after they look
right. `wz/Custom/export.txt` (item ids) dumps donor frames as repaint templates. The
client console prints `[AiSkin] WARNING` for oversized (>1024) or non-opaque materials.
Note: the launcher wipes `wz/Custom/` on NX sync — recreate the trigger files after.
