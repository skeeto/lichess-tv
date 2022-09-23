/* Chunk Parser
 *
 * This is a set of parser functions that help dealing with the
 * chunk data streamed from Lichess API.
 *
 * The chunk buffer is mutated in place, and the struct is populated
 * with null-terminated pointers into the chunk buffer. Since there are
 * no allocations, nothing needs to be freed.
 */

#include <string.h>
#include "chunk.h"

struct token {
    size_t off, len;
    enum {
        JSON_ERROR, JSON_STRING, JSON_NUMBER, JSON_COMMA, JSON_COLON,
        JSON_OBEG, JSON_OEND, JSON_ABEG, JSON_AEND, JSON_FALSE, JSON_TRUE
    } type;
};

enum symbol {
    SYM_d,
    SYM_t,
    SYM_featured,
    SYM_user,
    SYM_rating,
    SYM_black,
    SYM_color,
    SYM_fen,
    SYM_white,
    SYM_players,
    SYM_name,
    SYM_UNKNOWN
};

static enum symbol
parse_symbol(char *tok, size_t len)
{
    static const char s[49] =
        "dtfeatureduserratingblackcolorfenwhiteplayersname";
    static const struct {
        unsigned char off, len;
    } lut[16] = {
        { 0,  1}, { 1,  1}, { 2,  8}, {10,  4}, {14,  6}, {20,  5},
        {25,  5}, {30,  3}, {33,  5}, {38,  7}, {45,  4}
    };
    char tmp[4] = {0};
    memcpy(tmp, tok, len>4 ? 4 : len);
    unsigned h = (unsigned)tmp[3] << 24 | (unsigned)tmp[2] << 16 |
                 (unsigned)tmp[1] <<  8 | (unsigned)tmp[0] <<  0;
    int i = (h*2367153u)>>28 & 15;
    if (lut[i].len == len && !memcmp(s+lut[i].off, tok, len)) {
        return i;
    }
    return SYM_UNKNOWN;
}

static size_t
skip_whitespace(char *buf, size_t len, size_t off)
{
    while (off < len) {
        switch (buf[off]) {
        default  : return off;
        case '\t':
        case '\n':
        case '\r':
        case  ' ': off++;
        }
    }
    return off;
}

static size_t
skip_until(char *buf, size_t len, size_t off, int byte)
{
    for (; off < len; off++) {
        if (buf[off] == byte) {
            return off;
        }
    }
    return off;
}

static int
peek(char *buf, size_t len, size_t off)
{
    return off==len ? -1 : buf[off];
}

static struct token
json_next(char *buf, size_t len, size_t *poff)
{
    struct token t = {len, 0, JSON_ERROR};
    size_t off = skip_whitespace(buf, len, *poff);
    switch (peek(buf, len, off)) {
    case '{': t.off = off;
              t.len = 1;
              t.type = JSON_OBEG;
              *poff = off + 1;
              return t;
    case '}': t.off = off;
              t.len = 1;
              t.type = JSON_OEND;
              *poff = off + 1;
              return t;
    case '[': t.off = off;
              t.len = 1;
              t.type = JSON_ABEG;
              *poff = off + 1;
              return t;
    case ']': t.off = off;
              t.len = 1;
              t.type = JSON_AEND;
              *poff = off + 1;
              return t;
    case '"': t.off = off + 1;
              off = skip_until(buf, len, off+1, '"');
              // TODO: backslash handling
              t.len = off - t.off;
              if (off == len) {
                  *poff = off;
              } else {
                  t.type = JSON_STRING;
                  buf[off] = 0;
                  *poff = off + 1;
              }
              return t;
    case ',': t.off = off;
              t.len = 1;
              t.type = JSON_COMMA;
              *poff = off + 1;
              return t;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
              t.off = off;
              while (off < len && buf[off] >= '0' && buf[off] <= '9') {
                  off++;
              }
              t.len = off - t.off;
              t.type = JSON_NUMBER;
              *poff = off;
              return t;
    case ':': t.off = off;
              t.len = 1;
              t.type = JSON_COLON;
              *poff = off + 1;
              return t;
    case 'f': if (len-off >= 5 && !memcmp(buf+off, "false", 5)) {
                  t.off = off;
                  t.len = 5;
                  t.type = JSON_FALSE;
                  *poff = off + 5;
              } else {
                  *poff = len;
              }
              return t;
    case 't': if (len-off >= 4 && !memcmp(buf+off, "true", 4)) {
                  t.off = off;
                  t.len = 4;
                  t.type = JSON_TRUE;
                  *poff = off + 4;
              } else {
                  *poff = len;
              }
              return t;
    }
    *poff = off;
    return t;
}

static enum symbol
parse_key(char *buf, size_t len, size_t *poff)
{
    struct token t;
    t = json_next(buf, len, poff);
    if (t.type != JSON_STRING) {
        *poff = len;
        return SYM_UNKNOWN;
    }
    if (json_next(buf, len, poff).type != JSON_COLON) {
        *poff = len;
        return SYM_UNKNOWN;
    }
    return parse_symbol(buf+t.off, t.len);
}

static char *
parse_string(char *buf, size_t len, size_t *poff)
{
    struct token t = json_next(buf, len, poff);
    if (t.type != JSON_STRING) {
        *poff = len;
        return 0;
    }
    return buf + t.off;
}

static char *
parse_number(char *buf, size_t len, size_t *poff)
{
    struct token t = json_next(buf, len, poff);
    if (t.type != JSON_NUMBER) {
        *poff = len;
        return 0;
    }
    return buf + t.off;
}

static enum symbol
parse_enum(char *buf, size_t len, size_t *poff)
{
    struct token t = json_next(buf, len, poff);
    if (t.type != JSON_STRING) {
        *poff = len;
        return SYM_UNKNOWN;
    }
    return parse_symbol(buf+t.off, t.len);
}

static size_t
skip_value(char *buf, size_t len, size_t off)
{
    switch (json_next(buf, len, &off).type) {
    default:
        return len;
    case JSON_FALSE:
    case JSON_TRUE:
    case JSON_STRING:
    case JSON_NUMBER:
        return off;
    }
}

static char *
terminate_number(char *s)
{
    if (s) {
        char *p = s;
        while (*p >= '0' && *p <= '9') {
            p++;
        }
        *p = 0;
    }
    return s;
}

static size_t
parse_player(char *buf, size_t len, size_t off, struct player *p)
{
    int idx = -1;
    char *rating = 0;
    char *name = 0;

    if (json_next(buf, len, &off).type != JSON_OBEG) {
        return len;
    }
    for (;;) {
        switch (parse_key(buf, len, &off)) {
        default:
            off = skip_value(buf, len, off);
            break;
        case SYM_color:
            switch (parse_enum(buf, len, &off)) {
            case SYM_black: idx = 0; break;
            case SYM_white: idx = 1; break;
            default       : break;
            }
            break;
        case SYM_user:
            if (json_next(buf, len, &off).type != JSON_OBEG) {
                return len;
            }
            for (int done = 0; !done;) {
                switch (parse_key(buf, len, &off)) {
                default:
                    off = skip_value(buf, len, off);
                    break;
                case SYM_name:
                    name = parse_string(buf, len, &off);
                    break;
                }

                switch (json_next(buf, len, &off).type) {
                default        : return len;
                case JSON_OEND : done = 1; break;
                case JSON_COMMA: break;
                }
            }
            break;
        case SYM_rating:
            rating = parse_number(buf, len, &off);
            break;
        }

        switch (json_next(buf, len, &off).type) {
        default:
            return len;
        case JSON_OEND:
            if (idx == -1) {
                return len;
            }
            p[idx].name = name;
            // there is a byte definitely following the digits, and it's
            // no longer needed, so use it as a string terminator
            p[idx].rating = terminate_number(rating);
            return off;
        case JSON_COMMA:
            break;
        }
    }
}

static size_t
parse_data(char *buf, size_t len, size_t off, struct chunk *c)
{
    if (json_next(buf, len, &off).type != JSON_OBEG) {
        return len;
    }
    for (;;) {
        switch (parse_key(buf, len, &off)) {
        default:
            off = skip_value(buf, len, off);
            break;
        case SYM_fen:
            c->fen = parse_string(buf, len, &off);
            break;
        case SYM_players:
            if (json_next(buf, len, &off).type != JSON_ABEG) {
                return len;
            }
            off = parse_player(buf, len, off, c->players);
            if (json_next(buf, len, &off).type != JSON_COMMA) {
                return len;
            }
            off = parse_player(buf, len, off, c->players);
            if (json_next(buf, len, &off).type != JSON_AEND) {
                return len;
            }
            break;
        }

        switch (json_next(buf, len, &off).type) {
        default        : return len;
        case JSON_OEND : return off;
        case JSON_COMMA: break;
        }
    }
}

int
chunk_parse(char *buf, size_t len, struct chunk *c)
{
    size_t off = 0;
    if (json_next(buf, len, &off).type != JSON_OBEG) {
        return 0;
    }
    for (;;) {
        switch (parse_key(buf, len, &off)) {
        default:
            off = skip_value(buf, len, off);
            break;
        case SYM_t:
            switch (parse_enum(buf, len, &off)) {
            default:
                return 0;
            case SYM_featured:
                c->type = CHUNK_FEATURED;
                break;
            case SYM_fen:
                c->type = CHUNK_FEN;
                break;
            }
            break;
        case SYM_d:
            off = parse_data(buf, len, off, c);
            break;
        }

        switch (json_next(buf, len, &off).type) {
        default        : return 0;
        case JSON_OEND : return 1;
        case JSON_COMMA: break;
        }
    }
}
