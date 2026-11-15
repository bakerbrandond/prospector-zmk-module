#pragma once

#include <stdbool.h>
#include <stddef.h>

/*
 * Builds the layer roller's options: the "\n"-joined names of the visible
 * layers, in order, into buf (size len, always NUL-terminated). names[i] NULL
 * or "" shows the index as text. A name equal to `hidden` (may be NULL or "")
 * is skipped. Returns the visible row of layer `active`, or -1 if it is hidden
 * or out of range. Pure C so it can be host-tested (tests/roller).
 */
int roller_build(const char *const *names, int n, int active, const char *hidden, bool all_caps,
                 char *buf, size_t len);
