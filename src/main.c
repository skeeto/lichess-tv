#include <stdio.h>
#include "gfx.h"
#include "feed.h"
#include "chunk.h"
#include "fen.h"
#include "lib/debug.h"

void
on_data(char* chunk, size_t len)
{
    struct chunk c;
    if (!chunk_parse(chunk, len, &c)) {
        return;
    }
    if (c.type == CHUNK_FEATURED) {
        clear();
        gfx_draw_player_info(c.players);
    }
    char* fen   = c.fen;
    char* board = fen_to_board(fen);
    gfx_draw_board(board);
    refresh();
}

int
main()
{
    gfx_init();
    feed_init(on_data);
    gfx_destroy();
    return 0;
}
