////////////////////////////////////////////////////////////////////////////
//                          **** FAST-CHESS ****                          //
//                     Trivial Chess Playing Program                      //
//                    Copyright (c) 2020 David Bryant                     //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// fast-chess.h

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define FALSE (0)
#define TRUE (!FALSE)

#define BOARD_SIDE 8

/* board square masks */

#define PIECE   7
#define COLOR   8
#define MOVED   0x10
#define PINNED  0x40
#define BORDER  0x80

/* board piece values */

#define PAWN    2
#define KING    3
#define KNIGHT  4
#define BISHOP  5
#define ROOK    6
#define QUEEN   7

/* drawn game conditions */

#define STALEMATE       1
#define NO_MATE_POWER   2
#define MOVES_OVER_50   3
#define POSITION_3X     4

/* flags for eval_position() */

#define EVAL_BASE       0x1
#define EVAL_DEBUG      0x2
#define EVAL_POSITION   0x4
#define EVAL_PRUNE      0x8
#define EVAL_SCALE      0x10
#define EVAL_DECAY      0x20
#define EVAL_SCRAMBLE   0x40

typedef unsigned char square;

#define MAX_MATERIAL    55
#define MAX_MOVES       110
#define MAX_POS_IDS     50
#define MAX_CAP_POS     2

typedef struct {
    int move_number, move_color, in_check, drawn_game, reversable_moves, num_cap_pos;
    int white_king, white_material, white_pawns, white_epsquare;
    int black_king, black_material, black_pawns, black_epsquare;
    square board [(BOARD_SIDE + 4) * (BOARD_SIDE + 4)];
    int capture_positions [MAX_CAP_POS], position_ids [MAX_POS_IDS];
} FRAME;

typedef struct { int from, delta, promo; } MOVE;

#define DELTA(rank, file) ((rank) * (BOARD_SIDE + 4) + (file))
#define INDEX(rank, file) (((rank) + 1) * (BOARD_SIDE + 4) + (file) + 1)
#define SQUARE(frame, rank, file) ((frame)->board [INDEX ((rank), (file))])

#define DIAG1   (DELTA ( 1, 1))
#define DIAG2   (DELTA ( 1,-1))
#define DIAG3   (DELTA (-1, 1))
#define DIAG4   (DELTA (-1,-1))

#define ORTHOG1 (DELTA ( 0, 1))
#define ORTHOG2 (DELTA ( 0,-1))
#define ORTHOG3 (DELTA ( 1, 0))
#define ORTHOG4 (DELTA (-1, 0))

#define KNIGHT1 (DELTA ( 1, 2))
#define KNIGHT2 (DELTA ( 1,-2))
#define KNIGHT3 (DELTA (-1, 2))
#define KNIGHT4 (DELTA (-1,-2))
#define KNIGHT5 (DELTA ( 2, 1))
#define KNIGHT6 (DELTA ( 2,-1))
#define KNIGHT7 (DELTA (-2, 1))
#define KNIGHT8 (DELTA (-2,-1))

#define KINGOO  (DELTA ( 0, 2))
#define KINGOOO (DELTA ( 0,-2))

#define WPAWN1  (DELTA ( 1, 0))
#define WPAWN2  (DELTA ( 2, 0))
#define WPCAP1  (DELTA ( 1, 1))
#define WPCAP2  (DELTA ( 1,-1))
#define WPEPX1  (DELTA ( 0, 1))
#define WPEPX2  (DELTA ( 0,-1))

#define BPAWN1  (DELTA (-1, 0))
#define BPAWN2  (DELTA (-2, 0))
#define BPCAP1  (DELTA (-1, 1))
#define BPCAP2  (DELTA (-1,-1))
#define BPEPX1  (DELTA ( 0, 1))
#define BPEPX2  (DELTA ( 0,-1))

#define WPRANK 2
#define BPRANK 7

void init_frame (FRAME *frame);
void print_frame (FILE *out, FRAME *frame);
void print_square_name (FILE *out, int index);
void print_move (FILE *out, MOVE *move);
void print_game (FILE *out, MOVE *gameplay, int nmoves);
int input_square_name (char **in);
int input_move (char *in, MOVE *move);
int input_game (FILE *in, MOVE **gameplay, int *gameplay_moves);

int eval_position (FRAME *frame, MOVE *bestmove, int depth, int max_value, int flags);
int in_check (FRAME *frame);
int check_attack (square *dst, int color);
int generate_move_list (MOVE list [], FRAME *frame);
void execute_move (FRAME *frame, MOVE *move);
int sum_material (FRAME *frame, int color);
int count_pawns (FRAME *frame, int color);
int count_center_pawns (FRAME *frame, int color);
void scramble_moves (MOVE moves [], int nmoves);
int position_id (FRAME *frame);
