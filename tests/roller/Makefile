W = ../../boards/shields/prospector_adapter/src/widgets
test:
	cc -std=c11 -Wall -Wextra -Werror -I$(W) test_roller.c $(W)/roller_names.c -o test_roller && ./test_roller

.PHONY: test
