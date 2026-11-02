/*
 * json_helpers.h — Lightweight JSON String Builder
 *
 * PURPOSE:
 *   Builds JSON response strings without any external JSON library.
 *   Uses a simple buffer-based approach: write JSON tokens into a
 *   fixed-size character buffer, then output the entire string.
 *
 * NOTE:
 *   This is intentionally minimal — it handles the JSON output format
 *   needed by the parking garage API (objects, arrays, strings, numbers).
 *   It does NOT parse incoming JSON (the CLI uses positional arguments).
 */

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

/* Maximum size for a JSON response buffer */
#define JSON_BUF_SIZE 8192

typedef struct {
    char buffer[JSON_BUF_SIZE]; /* The JSON string being built               */
    int  pos;                   /* Current write position in the buffer      */
    int  depth;                 /* Nesting depth (for comma management)      */
    int  item_count[32];        /* Number of items at each nesting level     */
} JsonBuilder;

/*
 * json_init — Initialize the builder to produce a fresh JSON string.
 */
void json_init(JsonBuilder *jb);

/*
 * json_object_start / json_object_end — Write '{' and '}'.
 * @key: Optional key name if this object is a value in a parent object.
 *       Pass NULL for the root object or array elements.
 */
void json_object_start(JsonBuilder *jb, const char *key);
void json_object_end(JsonBuilder *jb);

/*
 * json_array_start / json_array_end — Write '[' and ']'.
 * @key: Key name for this array in the parent object, or NULL.
 */
void json_array_start(JsonBuilder *jb, const char *key);
void json_array_end(JsonBuilder *jb);

/*
 * json_add_string — Add a "key": "value" string pair.
 */
void json_add_string(JsonBuilder *jb, const char *key, const char *value);

/*
 * json_add_int — Add a "key": integer pair.
 */
void json_add_int(JsonBuilder *jb, const char *key, int value);

/*
 * json_add_long — Add a "key": long integer pair.
 */
void json_add_long(JsonBuilder *jb, const char *key, long value);

/*
 * json_add_double — Add a "key": floating-point pair (2 decimal places).
 */
void json_add_double(JsonBuilder *jb, const char *key, double value);

/*
 * json_finish — Null-terminate and return the built JSON string.
 * Returns pointer to the internal buffer (valid until next json_init).
 */
const char *json_finish(JsonBuilder *jb);

#endif /* JSON_HELPERS_H */
