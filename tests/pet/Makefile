CFLAGS = -std=c11 -Wall -Wextra -Werror -I../../src/pet

test:
	cc $(CFLAGS) test_core.c ../../src/pet/pet_core.c -o test_core && ./test_core

.PHONY: test
