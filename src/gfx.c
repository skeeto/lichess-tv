#include "gfx.h"
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

static int SCREEN_HEIGHT  = 24;
static int SCREEN_WIDTH   = 80;
static int BOARD_OFFSET_X = 0;
static int BOARD_OFFSET_Y = 0;

void
gfx_init()
{
    setlocale(LC_ALL, "");
    initscr();
    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }
    keypad(stdscr, TRUE);
    noecho();
    curs_set(false);
    use_default_colors();

    start_color();
    init_color(COLOR_BLACK, 70, 74, 94);
    init_color(COLOR_WHITE, 1000, 1000, 1000);
    init_color(COLOR_YELLOW, 509, 545, 721);
    init_color(COLOR_GREEN, 258, 278, 368);

    // black piece / white cell
    init_pair(1, COLOR_BLACK, COLOR_YELLOW);
    // black piece / green cell
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    // white piece / white cell
    init_pair(3, COLOR_WHITE, COLOR_YELLOW);
    // white piece / green cell
    init_pair(4, COLOR_WHITE, COLOR_GREEN);
    // board coordinate
    init_pair(5, COLOR_GREEN, -1);
    // description text
    init_pair(6, COLOR_YELLOW, -1);
    // icon black
    init_pair(7, COLOR_BLACK, -1);
    // icon white
    init_pair(8, COLOR_WHITE, -1);

    getmaxyx(stdscr, SCREEN_HEIGHT, SCREEN_WIDTH);
    BOARD_OFFSET_X = SCREEN_WIDTH / 2 - 10;
    BOARD_OFFSET_Y = SCREEN_HEIGHT / 2 - 6;
    if (BOARD_OFFSET_Y < 4)
        BOARD_OFFSET_Y = 4;

    refresh();
}

void
gfx_draw_board(char* board)
{
    attrset(COLOR_PAIR(5));
    for (int i = 0; i < 8; i++) {
        mvprintw(i + BOARD_OFFSET_Y, BOARD_OFFSET_X, "%d", 8 - i);
        mvprintw(8 + BOARD_OFFSET_Y, i * 2 + BOARD_OFFSET_X + 2, "%c", 'a' + i);
    }

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int index = row * 8 + col;
            int white_cell =
              row % 2 == 0 ? (col % 2 == 0 ? 1 : 0) : (col % 2 == 0 ? 0 : 1);
            char* piece = "  ";
            if (islower(board[index])) {
                // black piece
                attrset(COLOR_PAIR(white_cell ? 1 : 2));
            } else {
                // white piece
                attrset(COLOR_PAIR(white_cell ? 3 : 4));
            }
            switch (board[index]) {
                case 'p':
                case 'P':
                    piece = "♟";
                    break;
                case 'n':
                case 'N':
                    piece = "♞";
                    break;
                case 'b':
                case 'B':
                    piece = "♝";
                    break;
                case 'r':
                case 'R':
                    piece = "♜";
                    break;
                case 'q':
                case 'Q':
                    piece = "♛";
                    break;
                case 'k':
                case 'K':
                    piece = "♚";
                    break;
            }
            mvprintw(
              row + BOARD_OFFSET_Y, col * 2 + BOARD_OFFSET_X + 2, "%s ", piece
            );
        }
        mvprintw(row + BOARD_OFFSET_Y, 8 * 2 + BOARD_OFFSET_X + 2, "\n");
    }
}

void
gfx_draw_player_info(struct player* players)
{
    attrset(COLOR_PAIR(7));
    mvprintw(BOARD_OFFSET_Y - 2, BOARD_OFFSET_X, "●");
    attrset(COLOR_PAIR(6));
    mvprintw(BOARD_OFFSET_Y - 2, BOARD_OFFSET_X + 2, "%s", players[0].name);
    attrset(COLOR_PAIR(5));
    printw(" %s", players[0].rating);

    attrset(COLOR_PAIR(8));
    mvprintw(10 + BOARD_OFFSET_Y, BOARD_OFFSET_X, "●");
    attrset(COLOR_PAIR(6));
    mvprintw(10 + BOARD_OFFSET_Y, BOARD_OFFSET_X + 2, "%s", players[1].name);
    attrset(COLOR_PAIR(5));
    printw(" %s", players[1].rating);
}

void
gfx_destroy()
{
    endwin();
}