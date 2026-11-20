/* Host tests for the typing pet sprites. Run: make -C tests/pet test
 * Preview: make -C tests/pet preview */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pet_sprites.h"

static void test_all_sprites_present(void) {
    for (int s = 0; s < PET_STAGES; s++)
        for (int p = 0; p < PET_POSES; p++)
            assert(pet_sprite((enum pet_stage)s, (enum pet_pose)p) != NULL);
}

static void test_sprite_shape(void) {
    for (int s = 0; s < PET_STAGES; s++)
        for (int p = 0; p < PET_POSES; p++) {
            const char *const *rows = pet_sprite((enum pet_stage)s, (enum pet_pose)p);
            for (int y = 0; y < PET_SPRITE_H; y++) {
                assert(rows[y] != NULL);
                assert(strlen(rows[y]) == PET_SPRITE_W);
                assert(strspn(rows[y], ".#o*w") == PET_SPRITE_W);
            }
        }
}

static void test_sprite_not_blank(void) {
    for (int s = 0; s < PET_STAGES; s++)
        for (int p = 0; p < PET_POSES; p++) {
            const char *const *rows = pet_sprite((enum pet_stage)s, (enum pet_pose)p);
            int ink = 0;
            for (int y = 0; y < PET_SPRITE_H; y++)
                for (int x = 0; x < PET_SPRITE_W; x++)
                    ink += rows[y][x] != '.';
            assert(ink >= 20);
        }
}

static int same(const char *const *a, const char *const *b) {
    for (int y = 0; y < PET_SPRITE_H; y++)
        if (strcmp(a[y], b[y]) != 0)
            return 0;
    return 1;
}

static void test_stages_differ(void) {
    for (int a = 0; a < PET_STAGES; a++)
        for (int b = a + 1; b < PET_STAGES; b++)
            assert(!same(pet_sprite((enum pet_stage)a, PET_POSE_IDLE),
                         pet_sprite((enum pet_stage)b, PET_POSE_IDLE)));
}

#ifdef PREVIEW
static void preview(void) {
    static const char *const stage_names[] = {"Egg", "Baby", "Child", "Teen", "Adult"};
    static const char *const pose_names[] = {"idle", "bounce", "sleep", "hungry", "cheer"};
    for (int s = 0; s < PET_STAGES; s++)
        for (int p = 0; p < PET_POSES; p++) {
            printf("%s / %s\n", stage_names[s], pose_names[p]);
            const char *const *rows = pet_sprite((enum pet_stage)s, (enum pet_pose)p);
            for (int y = 0; y < PET_SPRITE_H; y++) {
                for (int x = 0; x < PET_SPRITE_W; x++) {
                    char c = rows[y][x];
                    fputs(c == '.' ? "  " : c == '#' ? "██" : c == 'o' ? "▒▒" : c == '*' ? "▓▓" : "░░",
                          stdout);
                }
                putchar('\n');
            }
        }
}
#endif

int main(void) {
    int n = 0;
#define RUN(t) (t(), n++)
    RUN(test_all_sprites_present);
    RUN(test_sprite_shape);
    RUN(test_sprite_not_blank);
    RUN(test_stages_differ);
    printf("ok %d tests\n", n);
#ifdef PREVIEW
    preview();
#endif
    return 0;
}
