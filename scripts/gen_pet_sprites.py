#!/usr/bin/env python3
"""Generate src/pet/pet_sprites.c: Clackling, the typing pet, drawn from shapes.

Run from the repo root:  python3 scripts/gen_pet_sprites.py > src/pet/pet_sprites.c
Preview the result:      make -C tests/pet preview

Palette chars: '.' transparent, '#' outline/dark, 'o' body, '*' accent, 'w' shine.
Clackling is a round critter with a keycap bump on its head. It grows ears
(Child), a curled tail (Teen) and keycap spines along its back (Adult).
"""
W = H = 24
STAGES = ["EGG", "BABY", "CHILD", "TEEN", "ADULT"]
POSES = ["IDLE", "BOUNCE", "SLEEP", "HUNGRY", "CHEER"]
GROUND = 22  # bottom row of the feet / egg

# body ellipse per stage: (rx, ry); centred on x 12, resting on GROUND
BODY = {"BABY": (5.0, 4.5), "CHILD": (6.5, 5.5), "TEEN": (7.5, 6.5), "ADULT": (8.5, 7.5)}


def blank():
    return [["."] * W for _ in range(H)]


def put(g, x, y, c):
    if 0 <= x < W and 0 <= y < H:
        g[y][x] = c


def ellipse(mask, cx, cy, rx, ry):
    for y in range(H):
        for x in range(W):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1.0:
                mask[y][x] = "o"


def mirror_put(g, x, y, c):
    """Put at x on the left and at its mirror on the right (pixels 11/12 are the middle)."""
    put(g, x, y, c)
    put(g, W - 1 - x, y, c)


def rect(mask, x0, y0, w, h, c="o"):
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            put(mask, x, y, c)


def outline(g):
    """Body pixels touching empty space (or the edge) become outline."""
    out = [row[:] for row in g]
    for y in range(H):
        for x in range(W):
            if g[y][x] != "o":
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if not (0 <= nx < W and 0 <= ny < H) or g[ny][nx] == ".":
                    out[y][x] = "#"
                    break
    return out


def shift(g, dy):
    out = blank()
    for y in range(H):
        for x in range(W):
            if 0 <= y + dy < H:
                out[y + dy][x] = g[y][x]
    return out


def egg(pose):
    g = blank()
    ellipse(g, 12, 13.5, 7, 9)
    g = outline(g)
    # keycap-legend zigzag band
    for x in range(6, 18):
        put(g, x, 14 if x % 4 < 2 else 15, "*")
    put(g, 9, 8, "w")
    put(g, 9, 9, "w")
    if pose == "BOUNCE":  # wobble: lean and hop
        g = shift(g, -1)
        put(g, 3, 20, "*"), put(g, 20, 20, "*")
    if pose == "CHEER":  # hatching crack
        for i, x in enumerate(range(7, 18)):
            put(g, x, 9 + (i % 2), "#")
        for x, y in ((3, 5), (20, 4), (21, 9), (2, 11)):
            put(g, x, y, "w")
    return g


def creature(stage, pose):
    rx, ry = BODY[stage]
    hungry = pose == "HUNGRY"
    if hungry:
        ry -= 0.5
    cy = GROUND + 1 - ry
    top = int(cy - ry)
    g = blank()
    ellipse(g, 12, cy, rx, ry)
    # keycap bump on the head
    cap_w = 6 if stage != "BABY" else 4
    rect(g, 12 - cap_w // 2, top - 2, cap_w, 3)
    # feet
    foot = 10 - int(rx * 0.6)
    rect(g, foot, GROUND - 1, 3, 2)
    rect(g, W - 3 - foot, GROUND - 1, 3, 2)
    if stage in ("CHILD", "TEEN", "ADULT"):  # pointed ears
        ey = top + 2
        for i in range(3):
            rect(g, 12 - int(rx) + i, ey - 2 + i, 1, 3 - i + 1)
            rect(g, 12 + int(rx) - 1 - i, ey - 2 + i, 1, 3 - i + 1)
    if stage in ("TEEN", "ADULT"):  # curled tail on the right
        tx, ty = 12 + int(rx), int(cy) + 1
        for x, y in ((0, 0), (1, 0), (2, -1), (2, -2), (1, -3)):
            rect(g, tx + x, ty + y, 1, 2)
    g = outline(g)
    # keycap legend
    put(g, 12, top - 1, "*")
    if stage == "ADULT":  # keycap spines along the back (left side)
        for x, y in ((3, int(cy) - 3), (2, int(cy)), (3, int(cy) + 3)):
            rect(g, x, y, 2, 2, "*")
    # face
    ex = max(2, int(rx * 0.45))
    eye_y = int(cy - ry * 0.2)
    big = stage != "BABY"
    a = 11 - ex  # left eye's inner column; the right eye mirrors it
    if pose in ("SLEEP", "HUNGRY"):  # closed / droopy: a short line
        for x in (a - 1, a):
            mirror_put(g, x, eye_y + 1, "#")
        if hungry:
            mirror_put(g, a, eye_y, "#")  # worried brow, raised inside
    elif pose == "CHEER":  # happy ^ eyes
        mirror_put(g, a - 2, eye_y + 1, "#")
        mirror_put(g, a - 1, eye_y, "#")
        mirror_put(g, a, eye_y + 1, "#")
    elif big:
        for x in (a - 1, a):
            mirror_put(g, x, eye_y, "#")
            mirror_put(g, x, eye_y + 1, "#")
        mirror_put(g, a - 1, eye_y, "w")
    else:
        mirror_put(g, a, eye_y, "#")
        mirror_put(g, a, eye_y + 1, "#")
    for cx in (a - 3, a - 2):  # cheeks, on the body only (never over the outline)
        if g[eye_y + 2][cx] == "o":
            mirror_put(g, cx, eye_y + 2, "*")
            break
    mouth_y = eye_y + 3
    if pose in ("HUNGRY", "CHEER"):  # open mouth
        rect(g, 11, mouth_y, 2, 2, "#")
    elif pose != "SLEEP":
        put(g, 11, mouth_y, "#"), put(g, 12, mouth_y, "#")
    if pose == "SLEEP":  # a little z
        for x, y in ((19, 2), (20, 2), (21, 2), (20, 3), (19, 4), (20, 4), (21, 4)):
            put(g, x, y, "w")
    if pose == "BOUNCE":
        g = shift(g, -2)
        for x, y in ((4, 22), (8, 23), (16, 23), (20, 22)):
            put(g, x, y, "*")
    if pose == "CHEER":
        g = shift(g, -1)
        for x, y in ((2, 3), (21, 5), (1, 12), (22, 13)):
            put(g, x, y, "w")
    return g


def sprite(stage, pose):
    return egg(pose) if stage == "EGG" else creature(stage, pose)


def main():
    print("/* Generated by scripts/gen_pet_sprites.py; edit that, not this file. */")
    print('#include "pet_sprites.h"\n')
    names = []
    for s in STAGES:
        for p in POSES:
            name = f"{s.lower()}_{p.lower()}"
            names.append(name)
            print(f"static const char *const {name}[PET_SPRITE_H] = {{")
            for row in sprite(s, p):
                print(f'    "{"".join(row)}",')
            print("};\n")
    print("static const char *const *const sprites[PET_STAGES][PET_POSES] = {")
    for i, s in enumerate(STAGES):
        row = ", ".join(names[i * len(POSES) : (i + 1) * len(POSES)])
        print(f"    {{{row}}},")
    print("};\n")
    print("const char *const *pet_sprite(enum pet_stage stage, enum pet_pose pose) {")
    print("    if ((unsigned)stage >= PET_STAGES || (unsigned)pose >= PET_POSES)")
    print("        return sprites[PET_EGG][PET_POSE_IDLE];")
    print("    return sprites[stage][pose];")
    print("}")


if __name__ == "__main__":
    main()
