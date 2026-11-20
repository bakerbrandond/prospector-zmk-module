#include "roller_names.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* append s (optionally uppercased) to buf, never past len-1 */
static void append(char *buf, size_t len, size_t *pos, const char *s, bool caps) {
    for (; *s && *pos + 1 < len; s++)
        buf[(*pos)++] = caps ? (char)toupper((unsigned char)*s) : *s;
    buf[*pos] = '\0';
}

/* true if name equals one entry of the comma-separated list (whole names only) */
static bool in_list(const char *name, const char *list) {
    size_t n = strlen(name);
    while (list && *list) {
        const char *end = strchr(list, ',');
        size_t len = end ? (size_t)(end - list) : strlen(list);
        if (len == n && n > 0 && strncmp(list, name, n) == 0)
            return true;
        list = end ? end + 1 : NULL;
    }
    return false;
}

int roller_build(const char *const *names, int n, int active, const char *hidden, bool all_caps,
                 char *buf, size_t len) {
    size_t pos = 0;
    int row = 0, sel = -1;
    if (len == 0)
        return -1;
    buf[0] = '\0';
    for (int i = 0; i < n; i++) {
        const char *name = names[i];
        if (name && in_list(name, hidden))
            continue;
        if (row > 0)
            append(buf, len, &pos, "\n", false);
        if (name && *name) {
            append(buf, len, &pos, name, all_caps);
        } else {
            char idx[12];
            snprintf(idx, sizeof(idx), "%d", i);
            append(buf, len, &pos, idx, false);
        }
        if (i == active)
            sel = row;
        row++;
    }
    return sel;
}
