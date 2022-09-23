#ifndef CHUNK_H
#define CHUNK_H

#include <stddef.h>

struct chunk {
    char *fen;
    struct player {
        char *name;
        char *rating;
    } players[2];
    enum {CHUNK_UNKNOWN, CHUNK_FEATURED, CHUNK_FEN} type;
};

int
chunk_parse(char *buf, size_t len, struct chunk *c);

#endif
