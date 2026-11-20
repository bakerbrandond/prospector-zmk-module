CFLAGS = -std=c11 -Wall -Wextra -Werror -I../../src/pet
CORE = ../../src/pet/pet_core.c
SPRITES = ../../src/pet/pet_sprites.c

test:
	cc $(CFLAGS) test_core.c $(CORE) -o test_core && ./test_core
	cc $(CFLAGS) test_sprites.c $(SPRITES) -o test_sprites && ./test_sprites

preview:
	cc $(CFLAGS) -DPREVIEW test_sprites.c $(SPRITES) -o test_sprites && ./test_sprites

.PHONY: test preview
