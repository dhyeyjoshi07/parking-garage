/*
 * json_helpers.c — Lightweight JSON String Builder
 *
 * See json_helpers.h for API documentation.
 *
 * APPROACH:
 *   We maintain a character buffer and write position. Each json_add_*
 *   function appends formatted text to the buffer. A depth counter and
 *   per-level item counter handle comma placement automatically:
 *   - First item at a nesting level: no comma prefix
 *   - Subsequent items: prepend a comma before the key-value pair
 *
 *   This avoids the common "trailing comma" JSON bug.
 */

#include "../include/json_helpers.h"
#include <stdio.h>    /* snprintf */
#include <string.h>   /* strlen */

/* ---------- Internal: append a raw string to the buffer ---------- */
static void jb_append(JsonBuilder *jb, const char *str)
{
    int len = (int)strlen(str);

    /* Safety check: don't overflow the buffer */
    if (jb->pos + len >= JSON_BUF_SIZE - 1) return;

    memcpy(jb->buffer + jb->pos, str, len);
    jb->pos += len;
}

/*
 * Internal: prepend a comma if this isn't the first item at the
 * current nesting level. This is how we avoid trailing commas —
 * we only add a comma BEFORE the second, third, etc. items.
 */
static void jb_comma(JsonBuilder *jb)
{
    if (jb->depth >= 0 && jb->item_count[jb->depth] > 0) {
        jb_append(jb, ",");
    }
    if (jb->depth >= 0) {
        jb->item_count[jb->depth]++;
    }
}

/* ---------- Internal: write a quoted key with colon ---------- */
static void jb_key(JsonBuilder *jb, const char *key)
{
    if (key) {
        jb_append(jb, "\"");
        jb_append(jb, key);
        jb_append(jb, "\":");
    }
}

/* ==================== Public API ==================== */

void json_init(JsonBuilder *jb)
{
    int i;
    if (!jb) return;

    jb->pos = 0;
    jb->depth = -1;
    for (i = 0; i < 32; i++) {
        jb->item_count[i] = 0;
    }
    jb->buffer[0] = '\0';
}

void json_object_start(JsonBuilder *jb, const char *key)
{
    if (!jb) return;

    /* If we're inside a parent, add comma between siblings */
    if (jb->depth >= 0) {
        jb_comma(jb);
    }

    jb_key(jb, key);
    jb_append(jb, "{");

    /* Enter a new nesting level */
    jb->depth++;
    if (jb->depth < 32) {
        jb->item_count[jb->depth] = 0;
    }
}

void json_object_end(JsonBuilder *jb)
{
    if (!jb) return;
    jb_append(jb, "}");
    jb->depth--;
}

void json_array_start(JsonBuilder *jb, const char *key)
{
    if (!jb) return;

    if (jb->depth >= 0) {
        jb_comma(jb);
    }

    jb_key(jb, key);
    jb_append(jb, "[");

    jb->depth++;
    if (jb->depth < 32) {
        jb->item_count[jb->depth] = 0;
    }
}

void json_array_end(JsonBuilder *jb)
{
    if (!jb) return;
    jb_append(jb, "]");
    jb->depth--;
}

void json_add_string(JsonBuilder *jb, const char *key, const char *value)
{
    if (!jb) return;
    jb_comma(jb);
    jb_key(jb, key);
    jb_append(jb, "\"");
    /* Simple escaping: replace " with \" in the value */
    if (value) {
        const char *p = value;
        while (*p) {
            if (*p == '"') {
                jb_append(jb, "\\\"");
            } else if (*p == '\\') {
                jb_append(jb, "\\\\");
            } else {
                char c[2] = {*p, '\0'};
                jb_append(jb, c);
            }
            p++;
        }
    }
    jb_append(jb, "\"");
}

void json_add_int(JsonBuilder *jb, const char *key, int value)
{
    char num[32];
    if (!jb) return;
    jb_comma(jb);
    jb_key(jb, key);
    snprintf(num, sizeof(num), "%d", value);
    jb_append(jb, num);
}

void json_add_long(JsonBuilder *jb, const char *key, long value)
{
    char num[32];
    if (!jb) return;
    jb_comma(jb);
    jb_key(jb, key);
    snprintf(num, sizeof(num), "%ld", value);
    jb_append(jb, num);
}

void json_add_double(JsonBuilder *jb, const char *key, double value)
{
    char num[32];
    if (!jb) return;
    jb_comma(jb);
    jb_key(jb, key);
    snprintf(num, sizeof(num), "%.2f", value);
    jb_append(jb, num);
}

const char *json_finish(JsonBuilder *jb)
{
    if (!jb) return "";
    jb->buffer[jb->pos] = '\0';
    return jb->buffer;
}
