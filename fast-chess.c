////////////////////////////////////////////////////////////////////////////
//                         **** FAST-CHESS ****                           //
//                     Trivial Chess Playing Program                      //
//                    Copyright (c) 2020 David Bryant                     //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// fast-chess.c

// This is a simple chess-playing program that I wrote a lifetime ago, just
// because I thought I could. The idea was if I could make the chess engine
// really fast by using a lot of inline code and making the final evaluation
// as simple as possible, I could process enough moves to make a really good
// player, but it never really worked out.
//
// I don't really know how good it is relative to other really simple chess
// programs, but at level 5 (which is tolerable) it provides me a challenge
// (which isn't saying much) and it doesn't seem to make really bad moves.
//
// It has no stored openings and it gets pretty lost in the endgame because
// it can't look far ahead enough to force a checkmate even with overwhelming
// material or recognize passed pawns until it's too late. So even if you
// screw up and lose in the middle game you can sometimes get a pawn promoted
// when it's not paying attention, or at least squeek out a draw by keeping
// your king in the center of the board.
//
// Another thing I wanted was for the program to not play the same game every
// time, so it randomly scrambles the moves before evaluating them (which
// results in different outcomes). This also makes it more interesting when
// it plays itself (which is a feature I've used extensively in development).
//
// To its credit it does handle all legal chess, including castling and its
// rules, en passent capture, and detecting draws based on the 50-move rule
// and 3-time position repitition.
//
// Good luck!

#include "fast-chess.h"

#ifdef _WIN32
#include <conio.h>
#else
int _kbhit (void), getch (void);
#endif

static unsigned int random_seed;

static const char *sign_on = "\n"
" FAST-CHESS  Trivial Chess Playing Program  Version 0.1\n"
" Copyright (c) 2020 David Bryant.  All Rights Reserved.\n\n";

static char *help = "\n\
 Options:\n\
  -R:     randomize so that we always start with a different game\n\
  -Gn:    specify number of games to play (otherwise stops on keypress)\n\
  -Wn:    computer plays white at level n (1 to about 6; higher is slower)\n\
  -Bn:    computer plays black at level n (1 to about 6; higher is slower)\n\
  -En:    evaluate available moves at level n and display scores (default=2)\n\n\
 Commands:\n\
  H <cr>:        print this message\n\
  W n <cr>:      computer plays white at level n\n\
  B n <cr>:      computer plays black at level n\n\
  W <cr>:        returns white play to user\n\
  B <cr>:        returns black play to user\n\
  S <file><cr>:  save game to specified file\n\
  L <file><cr>:  load game from specified file\n\
  R <cr>:        resign game and start new game\n\
  Q <cr>:        resign game and quit\n\n";

int main (argc, argv) int argc; char **argv;
{
    int games_to_play = 0, games = 0, whitewins = 0, blackwins = 0, draws = 0, whitedraws = 0, blackdraws = 0;
    int nmoves, mindex, maxmoves = 0, minmoves = 1000, asked4help = FALSE, quit = FALSE, resign;
    int white_level = 0, black_level = 0, level;
    MOVE bestmove, moves [MAX_MOVES + 10];
    time_t start_time, stop_time;
    char command [81], *cptr;
    long totalmoves = 0;
    FRAME frame;
    FILE *file;

    while (--argc) {

        if (**++argv == '-' || **argv == '/')
            switch (*++*argv) {

                case 'H': case 'h':
                    asked4help = TRUE;
                    break;

                case 'G': case 'g':
                    games_to_play = atoi (++*argv);
                    break;

                case 'W': case 'w':
                    white_level = atoi (++*argv);
                    break;

                case 'B': case 'b':
                    black_level = atoi (++*argv);
                    break;

                case 'R': case 'r':
                    srand (random_seed = atoi (++*argv));

                    if (!random_seed) {
                        random_seed = time (NULL);
                        srand (time (NULL));
                    }
                    break;

                default:
                    fprintf (stderr, "illegal option: %s\n%s", --*argv, help);
                    exit (1);
            }
        else {
            fprintf (stderr, "argument ignored: %s\n%s", *argv, help);
            exit (1);
        }
    }

    printf ("%s", sign_on);

    if (asked4help)
        printf ("%s", help);

    time (&start_time);

    while (!quit && (!white_level || !black_level || !_kbhit())) {
        init_frame (&frame);

        while (!frame.drawn_game) {

            level = (frame.move_color) ? black_level : white_level;
            bestmove.from = 0;

            if (level > 0)
                eval_position (&frame, &bestmove, level, 20000, EVAL_BASE | EVAL_POSITION | EVAL_SCALE | EVAL_PRUNE | EVAL_DECAY);
            else if ((nmoves = generate_move_list (moves, &frame)) != 0) {
                if (nmoves > MAX_MOVES) {
                    print_frame (stdout, &frame);
                    fprintf (stderr, "%d legal moves!\n", nmoves);
                    exit (1);
                }

                if (!level) {
                    print_frame (stdout, &frame);
                    fprintf (stderr, "input move or command: ");
                    resign = FALSE;

                    for (cptr = command; (*cptr = fgetc (stdin)) != '\n'; ++cptr);

                    if (cptr == command)
                        continue;

                    *cptr = '\0';
                    cptr = command;

                    if (input_move (cptr, &bestmove)) {

                        for (mindex = 0; mindex < nmoves; ++mindex)
                            if (bestmove.from == moves [mindex].from &&
                                bestmove.delta == moves [mindex].delta) {

                                    if (moves [mindex].promo && !bestmove.promo)
                                        bestmove.promo = QUEEN;

                                    break;
                            }

                        if (mindex == nmoves) {
                            fprintf (stderr, "\ninvalid move!\n\007");
                            continue;
                        }
                    }
                    else {
                        int eval_level;

                        while (*++cptr && *cptr == ' ');

                        switch (command [0]) {

                            default:
                                fprintf (stderr, "illegal command\n\007");

                            case 'H': case 'h':
                                fprintf (stderr, "%s", help);
                                break;

                            case 'Q': case 'q':
                                quit = TRUE;
                                break;

                            case 'R': case 'r':
                                resign = TRUE;
                                break;

                            case 'W': case 'w':
                                white_level = atoi (cptr);
                                break;

                            case 'B': case 'b':
                                black_level = atoi (cptr);
                                break;

                            case 'E': case 'e':
                                eval_level = atoi (cptr);
                                if (eval_level < 1) eval_level = 2;
                                eval_position (&frame, NULL, eval_level, 20000, EVAL_DEBUG | EVAL_BASE | EVAL_POSITION | EVAL_SCALE | EVAL_DECAY);
                                break;

                            case 'S': case 's':
                                if (!*cptr) {
                                    fprintf (stderr, "need filename\n\007");
                                    break;
                                }

                                if ((file = fopen (cptr, "wb")) == NULL) {
                                    fprintf (stderr, "can't open file %s\n\007", cptr);
                                    break;
                                }

                                fwrite (&frame, sizeof (frame), 1, file);
                                fclose (file);
                                break;

                            case 'L': case 'l':
                                if (!*cptr) {
                                    fprintf (stderr, "need filename\n\007");
                                    break;
                                }

                                if ((file = fopen (cptr, "rb")) == NULL) {
                                    fprintf (stderr, "can't open file %s\n\007", cptr);
                                    break;
                                }

                                if (!fread (&frame, sizeof (frame), 1, file))
                                    fprintf (stderr, "can't read file %s\n\007", cptr);

                                fclose (file);
                                break;
                        }

                        if (resign || quit) {
                            fprintf (stderr, "are you sure (y or n) ? ");
                            for (cptr = command; (*cptr = fgetc (stdin)) != '\n'; ++cptr);
                            *cptr = '\0';
                            cptr = command;

                            if (tolower (*cptr) != 'y')
                                continue;
                        }
                        else
                            continue;
                    }
                }
                else
                    bestmove = moves [rand () % nmoves];
            }
            else if (!frame.in_check)
                frame.drawn_game = STALEMATE;

            if (bestmove.from) {
                if (frame.move_color) {
                    print_move (stdout, &bestmove);
                    putchar ('\n');
                }
                else {
                    printf ("%3d: ", frame.move_number);
                    print_move (stdout, &bestmove);
                    fflush (stdout);
                }

                execute_move (&frame, &bestmove);
            }
            else
                break;
        }

        if (frame.move_number > 1 || frame.move_color) {
            if (frame.drawn_game) {
                draws++;

                if (frame.white_material > frame.black_material)
                    ++whitedraws;
                else if (frame.black_material > frame.white_material)
                    ++blackdraws;
            }
            else
                frame.move_color ? ++whitewins : ++blackwins;

            totalmoves += (2 * frame.move_number) - (frame.move_color ? 1 : 2);

            if (frame.move_number < minmoves)
                minmoves = frame.move_number;

            if (frame.move_number > maxmoves)
                maxmoves = frame.move_number;

            if (frame.white_material != sum_material (&frame, 0) ||
                frame.black_material != sum_material (&frame, COLOR) ||
                frame.white_pawns != count_pawns (&frame, 0) ||
                frame.black_pawns != count_pawns (&frame, COLOR)) {
                    fprintf (stderr, "material sum error!\n");
                    exit (1);
            }

            print_frame (stdout, &frame);
            printf ("-------------------------------------");
            printf ("-------------------------------------\n");

            ++games;

            if (games_to_play && games_to_play == games)
                break;
        }
    }

    time (&stop_time);

    if (_kbhit ())
        getch ();

    if (!games)
        exit (0);

    if (games == 1) {
        if (whitewins || blackwins)
            printf ("1 game, %s won\n", whitewins ? "white" : "black");
        else if (whitedraws || blackdraws)
            printf ("1 drawn game, but %s was ahead in material\n", whitedraws ? "white" : "black");
        else
            printf ("1 drawn game\n");
    }
    else if (draws) {
        printf ("%d games total, white won %d and black won %d, %d %s drawn\n",
            games, whitewins, blackwins, draws, draws == 1 ? "was" : "were");

        if (whitedraws && blackdraws)
            printf ("of the drawn games, white was ahead in material in %d and black was in %d\n", whitedraws, blackdraws);
        else if (whitedraws)
            printf ("of the drawn games, white was ahead in material in %d\n", whitedraws);
        else if (blackdraws)
            printf ("of the drawn games, black was ahead in material in %d\n", blackdraws);
    }
    else
        printf ("%d games total, white won %d and black won %d\n", games, whitewins, blackwins);

    printf ("%ld total moves made\n", totalmoves);
    printf ("%u max moves per game\n", maxmoves);
    printf ("%u min moves per game\n", minmoves);
    printf ("play time: %ld seconds\n", stop_time - start_time);

    return 0;
}

void init_frame (FRAME *frame)
{
    int initial_lineup [] = {
        ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK };

    int rank, file;

    memset (frame->board, BORDER, sizeof (frame->board));

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            SQUARE (frame, rank, file) = 0;

    for (file = 1; file <= BOARD_SIDE; ++file) {

        SQUARE (frame, 1, file) = SQUARE (frame, BOARD_SIDE, file) =
                initial_lineup [file - 1];

        SQUARE (frame, 2, file) = SQUARE (frame, BOARD_SIDE - 1, file) = PAWN;
    }

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            if (SQUARE (frame, rank, file)) {

                if (rank > BOARD_SIDE / 2)
                    SQUARE (frame, rank, file) |= COLOR;

                if ((SQUARE (frame, rank, file) & PIECE) == KING) {

                    if (SQUARE (frame, rank, file) & COLOR)
                        frame->black_king = INDEX (rank, file);
                    else
                        frame->white_king = INDEX (rank, file);
                }
            }

    frame->drawn_game = frame->white_epsquare = frame->black_epsquare = 0;
    frame->reversable_moves = frame->move_color = 0;
    frame->move_number = 1;

    frame->in_check = in_check (frame);
    frame->position_ids [frame->reversable_moves] = position_id (frame);
    frame->black_material = sum_material (frame, COLOR);
    frame->white_material = sum_material (frame, 0);
    frame->black_pawns = count_pawns (frame, COLOR);
    frame->white_pawns = count_pawns (frame, 0);
    frame->num_cap_pos = 0;
}

int eval_position (FRAME *frame, MOVE *bestmove, int depth, int max_value, int flags)
{
    int nmoves, mindex, value, min_value;
    MOVE moves [MAX_MOVES + 10];
    FRAME temp;

    if ((flags & EVAL_BASE) && frame->move_number == 1 && !frame->move_color)
        depth = 2;

    if (flags & EVAL_DECAY)
        max_value += (max_value + 128) >> 8;

    if ((flags & EVAL_BASE) && (flags & EVAL_SCALE)) {
        int total_material = frame->white_material + frame->black_material;

        if (total_material < 40) depth++;
        if (total_material < 20) depth++;
        if (total_material < 10) depth++;
    }

    if (depth < -24) {
        print_frame (stderr, frame);
        fprintf (stderr, "depth = %d!\n", depth);
        exit (1);
    }

    if (frame->drawn_game)
        return 0;

    if (!(nmoves = generate_move_list (moves, frame))) {
        if (frame->in_check)
            return -10000;
        else {
            frame->drawn_game = STALEMATE;
            return 0;
        }
    }

    if (nmoves > MAX_MOVES) {
        print_frame (stdout, frame);
        printf ("%d legal moves!\n", nmoves);
        exit (1);
    }

    scramble_moves (moves, nmoves);

    if (depth > 0 || frame->in_check) {
        min_value = 20000;

        if (flags & EVAL_DEBUG)
            printf ("\n");

        for (mindex = 0; mindex < nmoves; ++mindex) {

            temp = *frame;
            execute_move (&temp, moves + mindex);
            value = eval_position (&temp, NULL, depth-1, min_value, flags & ~(EVAL_DEBUG | EVAL_BASE));

            if (flags & EVAL_DEBUG) {
                printf ("%d: ", mindex + 1);
                print_move (stdout, moves + mindex);
                printf ("value = %d\n", -value);
            }

            if (value < min_value) {
                min_value = value;

                if (bestmove)
                    *bestmove = moves [mindex];

                if ((flags & EVAL_PRUNE) && -min_value >= max_value)
                    break;
            }
        }

        if (flags & EVAL_DEBUG)
            printf ("\n");
    }
    else {
        if (frame->white_material > MAX_MATERIAL || frame->black_material > MAX_MATERIAL)
            fprintf (stderr, "warning: material too high!\n");

        if (frame->white_material > frame->black_material)
            min_value = (frame->white_material + 10) * 500 /
                (frame->black_material + 10) - 500;
        else
            min_value = -((frame->black_material + 10) * 500 /
                (frame->white_material + 10) - 500);

        if (!frame->move_color)
            min_value = -min_value;

        if (flags & EVAL_POSITION) {
            min_value += count_center_pawns (frame, frame->move_color ^ COLOR) * 2;
            min_value -= count_center_pawns (frame, frame->move_color) * 2;
            frame->move_color ^= COLOR;
            min_value += generate_move_list (NULL, frame) - nmoves;
            frame->move_color ^= COLOR;
        }

        for (mindex = 0; mindex < nmoves; ++mindex) {
            int dest = moves [mindex].from + moves [mindex].delta, cindex;

            if ((flags & EVAL_PRUNE) && -min_value >= max_value)
                break;

            if (!frame->board [dest])
                continue;

            temp = *frame;

            for (cindex = 0; cindex < temp.num_cap_pos; ++cindex)
                if (temp.capture_positions [cindex] == dest)
                    break;

            if (cindex == temp.num_cap_pos) {
                if (cindex < MAX_CAP_POS)
                    temp.capture_positions [temp.num_cap_pos++] = dest;
                else
                    continue;
            }

            execute_move (&temp, moves + mindex);
            value = eval_position (&temp, NULL, depth-1, min_value, flags & ~(EVAL_DEBUG | EVAL_BASE));

            if (value < min_value)
                min_value = value;
        }
    }

    if (flags & EVAL_DECAY)
        min_value -= (min_value + 128) >> 8;

    return -min_value;
}

int in_check (FRAME *frame)
{
    int kindex = frame->move_color ? frame->black_king : frame->white_king;
    return check_attack (&frame->board [kindex], frame->move_color ^ COLOR);
}

#define attackpath(dir, mask)                                   \
    if ((*(src = dst + dir) & (PIECE | COLOR)) == ktest)        \
        return TRUE;                                            \
    else {                                                      \
        while (!*src)                                           \
            src += dir;                                         \
                                                                \
        if ((*src & (mask | COLOR)) == stest)                   \
            return TRUE;                                        \
    }                                                   

int check_attack (square *dst, int color)
{
    int ktest, stest;
    square *src;

    if (color) {
        if ((dst [-BPCAP1] & (PIECE | COLOR)) == (PAWN | COLOR) ||
            (dst [-BPCAP2] & (PIECE | COLOR)) == (PAWN | COLOR))
                return TRUE;
    }
    else {
        if ((dst [-WPCAP1] & (PIECE | COLOR)) == PAWN ||
            (dst [-WPCAP2] & (PIECE | COLOR)) == PAWN)
                return TRUE;
    }

    ktest = KNIGHT | color;

    if ((dst [KNIGHT1] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT2] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT3] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT4] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT5] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT6] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT7] & (PIECE | COLOR)) == ktest ||
        (dst [KNIGHT8] & (PIECE | COLOR)) == ktest)
            return TRUE;

    ktest = KING | color;
    stest = BISHOP | color;

    attackpath (DIAG1, BISHOP);
    attackpath (DIAG2, BISHOP);
    attackpath (DIAG3, BISHOP);
    attackpath (DIAG4, BISHOP);

    stest = ROOK | color;

    attackpath (ORTHOG1, ROOK);
    attackpath (ORTHOG2, ROOK);
    attackpath (ORTHOG3, ROOK);
    attackpath (ORTHOG4, ROOK);

    return FALSE;
}

#define pinpath(dir, mask)                              \
    for (pin = dst + dir; !*pin; pin += dir);           \
                                                        \
    if ((*pin & PIECE) && !((*dst ^ *pin) & COLOR)) {   \
                                                        \
        for (src = pin + dir; !*src; src += dir);       \
                                                        \
        if ((*src & (mask | COLOR)) == test)            \
            *pin |= PINNED;                             \
    }                                                   \

void set_pinned_status (FRAME *frame)
{
    int kindex = frame->move_color ? frame->black_king : frame->white_king;
    square *dst = &frame->board [kindex], *pin, *src;
    int rank, file, test;

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            if (SQUARE (frame, rank, file) & PINNED)
                SQUARE (frame, rank, file) &= ~PINNED;

    test = BISHOP | (~*dst & COLOR);

    pinpath (DIAG1, BISHOP);
    pinpath (DIAG2, BISHOP);
    pinpath (DIAG3, BISHOP);
    pinpath (DIAG4, BISHOP);

    test = ROOK | (~*dst & COLOR);

    pinpath (ORTHOG1, ROOK);
    pinpath (ORTHOG2, ROOK);
    pinpath (ORTHOG3, ROOK);
    pinpath (ORTHOG4, ROOK);
}

#define genmove(dir)                                    \
    if (!*(dst = src + (move.delta = dir)) ||           \
        ((*dst & PIECE) && ((*dst ^ *src) & COLOR)))    \
            *listptr++ = move;                          \

#define checkmove(dir)                                  \
    if (!*(dst = src + (move.delta = dir)) ||           \
        ((*dst & PIECE) && ((*dst ^ *src) & COLOR))) {  \
                                                        \
            capture_temp = *dst; *dst = *src; *src = 0; \
                                                        \
            if (!in_check (frame))                      \
                *listptr++ = move;                      \
                                                        \
            *src = *dst; *dst = capture_temp;           \
    }

#define genpath(dir)                                    \
    for (move.delta = 0;                                \
        !*(dst = src + (move.delta += dir)) ||          \
        ((*dst & PIECE) && ((*dst ^ *src) & COLOR));) { \
                                                        \
            *listptr++ = move;                          \
                                                        \
            if (*dst)                                   \
                break;                                  \
    }

#define checkpath(dir)                                  \
    for (move.delta = 0;                                \
        !*(dst = src + (move.delta += dir)) ||          \
        ((*dst & PIECE) && ((*dst ^ *src) & COLOR));) { \
                                                        \
            capture_temp = *dst; *dst = *src; *src = 0; \
                                                        \
            if (!in_check (frame))                      \
                *listptr++ = move;                      \
                                                        \
            if (*src = *dst, *dst = capture_temp)       \
                break;                                  \
    }

#define genkmove(dir)                                   \
    if (!*(dst = src + (move.delta = dir)) ||           \
        ((*dst & PIECE) && ((*dst ^ *src) & COLOR))) {  \
                                                        \
            capture_temp = *dst; *dst = *src; *src = 0; \
                                                        \
            if (!check_attack (dst, ~*dst & COLOR))     \
                *listptr++ = move;                      \
                                                        \
            *src = *dst; *dst = capture_temp;           \
    }

#define checkpcap(dir, startrank)                       \
    if ((*(dst = src + (move.delta = dir)) & PIECE) &&  \
        ((*dst ^ *src) & COLOR)) {                      \
                                                        \
            capture_temp = *dst; *dst = *src; *src = 0; \
                                                        \
            if (!in_check (frame)) {                    \
                if (rank == 9 - (startrank))            \
                    for (move.promo = KNIGHT;           \
                        move.promo &= PIECE;            \
                        ++move.promo)                   \
                            *listptr++ = move;          \
                else                                    \
                    *listptr++ = move;                  \
            }                                           \
                                                        \
            *src = *dst; *dst = capture_temp;           \
    }

#define genpcap(dir, startrank)                         \
    if ((*(dst = src + (move.delta = dir)) & PIECE) &&  \
        ((*dst ^ *src) & COLOR)) {                      \
            if (rank == 9 - (startrank))                \
                for (move.promo = KNIGHT;               \
                    move.promo &= PIECE; ++move.promo)  \
                        *listptr++ = move;              \
            else                                        \
                *listptr++ = move;                      \
    }

#define genpepx(dir, epdir, epsqr)                      \
    if (move.from + epdir == epsqr) {                   \
                                                        \
        *(dst = src + (move.delta = dir)) = *src;       \
        capture_temp = *(cap = &frame->board [epsqr]);  \
        *cap = *src = 0;                                \
                                                        \
        if (!in_check (frame))                          \
            *listptr++ = move;                          \
                                                        \
        *cap = capture_temp;                            \
        *src = *dst;                                    \
        *dst = 0;                                       \
    }

#define checkpmove(dir, startrank)                      \
    if (!*(dst = src + (move.delta = dir))) {           \
                                                        \
        *dst = *src; *src = 0;                          \
                                                        \
        if (!in_check (frame)) {                        \
                                                        \
            if (rank == (9 - (startrank)))              \
                for (move.promo = KNIGHT;               \
                    move.promo &= PIECE; ++move.promo)  \
                        *listptr++ = move;              \
            else                                        \
                *listptr++ = move;                      \
        }                                               \
                                                        \
        *src = *dst; *dst = 0;                          \
                                                        \
        if (rank == (startrank) &&                      \
            !*(dst = src + (move.delta += dir))) {      \
                                                        \
                *dst = *src; *src = 0;                  \
                                                        \
                if (!in_check (frame))                  \
                    *listptr++ = move;                  \
                                                        \
                *src = *dst; *dst = 0;                  \
            }                                           \
    }

#define genpmove(dir, startrank)                        \
    if (!src [move.delta = dir]) {                      \
        if (rank == (9 - (startrank)))                  \
            for (move.promo = KNIGHT;                   \
                move.promo &= PIECE; ++move.promo)      \
                    *listptr++ = move;                  \
        else {                                          \
            *listptr++ = move;                          \
                                                        \
            if (rank == (startrank) &&                  \
                !src [move.delta += dir])               \
                    *listptr++ = move;                  \
        }                                               \
    }

MOVE null_list [MAX_MOVES + 10];

int generate_move_list (MOVE list [], FRAME *frame)
{
    square *src, *dst, *cap, capture_temp;
    MOVE *listptr, move;
    int rank, file;

    if (!list)
        listptr = list = null_list;
    else
        listptr = list;

    if (!frame->in_check)
        set_pinned_status (frame);

    move.promo = 0;

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file) {

            src = &frame->board [move.from = INDEX (rank, file)];

            if ((*src & COLOR) == frame->move_color)
                switch (*src & (PIECE | COLOR)) {

                    case BISHOP | COLOR:
                    case QUEEN | COLOR:
                    case BISHOP:
                    case QUEEN:

                        if (frame->in_check || (*src & PINNED)) {

                            checkpath (DIAG1);
                            checkpath (DIAG2);
                            checkpath (DIAG3);
                            checkpath (DIAG4);
                        }
                        else {
                            genpath (DIAG1);
                            genpath (DIAG2);
                            genpath (DIAG3);
                            genpath (DIAG4);
                        }

                        if ((*src & PIECE) == BISHOP)
                            break;

                    case ROOK | COLOR:
                    case ROOK:

                        if (frame->in_check || (*src & PINNED)) {

                            checkpath (ORTHOG1);
                            checkpath (ORTHOG2);
                            checkpath (ORTHOG3);
                            checkpath (ORTHOG4);
                        }
                        else {
                            genpath (ORTHOG1);
                            genpath (ORTHOG2);
                            genpath (ORTHOG3);
                            genpath (ORTHOG4);
                        }

                        break;

                    case KNIGHT | COLOR:
                    case KNIGHT:

                        if (frame->in_check) {

                            checkmove (KNIGHT1);
                            checkmove (KNIGHT2);
                            checkmove (KNIGHT3);
                            checkmove (KNIGHT4);
                            checkmove (KNIGHT5);
                            checkmove (KNIGHT6);
                            checkmove (KNIGHT7);
                            checkmove (KNIGHT8);
                        }
                        else if (!(*src & PINNED)) {

                            genmove (KNIGHT1);
                            genmove (KNIGHT2);
                            genmove (KNIGHT3);
                            genmove (KNIGHT4);
                            genmove (KNIGHT5);
                            genmove (KNIGHT6);
                            genmove (KNIGHT7);
                            genmove (KNIGHT8);
                        }

                        break;

                    case KING | COLOR:
                    case KING:

                        genkmove (ORTHOG1);
                        genkmove (ORTHOG2);
                        genkmove (ORTHOG3);
                        genkmove (ORTHOG4);
                        genkmove (DIAG1);
                        genkmove (DIAG2);
                        genkmove (DIAG3);
                        genkmove (DIAG4);

                        if (!frame->in_check && !(*src & MOVED)) {

                            if (!src [1] && !src [2] &&
                                ((src [3] & (PIECE | MOVED)) == ROOK) &&
                                !check_attack (src + 1, ~*src & COLOR) &&
                                !check_attack (src + 2, ~*src & COLOR)) {

                                    move.delta = KINGOO;
                                    *listptr++ = move;
                            }

                            if (!src [-1] && !src [-2] && !src [-3] &&
                                ((src [-4] & (PIECE | MOVED)) == ROOK) &&
                                !check_attack (src - 1, ~*src & COLOR) &&
                                !check_attack (src - 2, ~*src & COLOR)) {

                                    move.delta = KINGOOO;
                                    *listptr++ = move;
                            }
                        }

                        break;

                    case PAWN | COLOR:

                        if (frame->in_check || (*src & PINNED)) {
                            checkpmove (BPAWN1, BPRANK);
                            checkpcap (BPCAP1, BPRANK);
                            checkpcap (BPCAP2, BPRANK);
                        }
                        else {
                            genpmove (BPAWN1, BPRANK);
                            genpcap (BPCAP1, BPRANK);
                            genpcap (BPCAP2, BPRANK);
                        }

                        genpepx (BPCAP1, BPEPX1, frame->white_epsquare);
                        genpepx (BPCAP2, BPEPX2, frame->white_epsquare);
                        break;

                    case PAWN:

                        if (frame->in_check || (*src & PINNED)) {
                            checkpmove (WPAWN1, WPRANK);
                            checkpcap (WPCAP1, WPRANK);
                            checkpcap (WPCAP2, WPRANK);
                        }
                        else {
                            genpmove (WPAWN1, WPRANK);
                            genpcap (WPCAP1, WPRANK);
                            genpcap (WPCAP2, WPRANK);
                        }

                        genpepx (WPCAP1, WPEPX1, frame->black_epsquare);
                        genpepx (WPCAP2, WPEPX2, frame->black_epsquare);
                        break;
                }
        }

    return listptr - list;
}

static int piece_value [] = { 0, 0, 1, 0, 3, 3, 5, 9 };

void execute_move (FRAME *frame, MOVE *move)
{
    square *src = &frame->board [move->from];
    square *dst = src + move->delta;
    square *cap = dst;

    frame->drawn_game = 0;

    if ((*cap & PIECE) == KING) {
        print_frame (stdout, frame);
        printf ("capturing a king!\n");
        exit (1);
    }

    if ((*src & PIECE) == PAWN || *dst)
        frame->reversable_moves = 0;
    else
        ++frame->reversable_moves;

    if ((*src & PIECE) == KING) {

        if (move->delta == KINGOO) {
            src [1] = src [3] | MOVED;
            src [3] = 0;
        }
        else if (move->delta == KINGOOO) {
            src [-1] = src [-4] | MOVED;
            src [-4] = 0;
        }

        if (*src & COLOR)
            frame->black_king = move->from + move->delta;
        else
            frame->white_king = move->from + move->delta;
    }
    else if ((*src & PIECE) == PAWN && !*cap) {

        if (*src & COLOR) {
            if (move->delta != BPAWN1 && move->delta != BPAWN2)
                cap = &frame->board [frame->white_epsquare];
        }
        else if (move->delta != WPAWN1 && move->delta != WPAWN2)
            cap = &frame->board [frame->black_epsquare];
    }

    if ((*src & PIECE) == PAWN && (*src & COLOR) && move->delta == BPAWN2)
        frame->black_epsquare = move->from + move->delta;
    else
        frame->black_epsquare = 0;

    if ((*src & PIECE) == PAWN && !(*src & COLOR) && move->delta == WPAWN2)
        frame->white_epsquare = move->from + move->delta;
    else
        frame->white_epsquare = 0;

    if (*cap) {
        if (*cap & COLOR) {
            frame->black_material -= piece_value [*cap & PIECE];

            if ((*cap & PIECE) == PAWN)
                frame->black_pawns--;
        }
        else {
            frame->white_material -= piece_value [*cap & PIECE];

            if ((*cap & PIECE) == PAWN)
                frame->white_pawns--;
        }

        *cap = 0;
    }

    if (move->promo) {
        if ((*dst = move->promo | (*src & COLOR) | MOVED) & COLOR) {
            (frame->black_material += piece_value [*dst & PIECE] - 1);
            frame->black_pawns--;
        }
        else {
            (frame->white_material += piece_value [*dst & PIECE] - 1);
            frame->white_pawns--;
        }
    }
    else
        *dst = *src | MOVED;

    *src = 0;

    if (!(frame->move_color ^= COLOR))
        ++frame->move_number;

    frame->in_check = in_check (frame);

    if (frame->reversable_moves < MAX_POS_IDS) {

        int pindex, repeat, pos = position_id (frame);

        frame->position_ids [frame->reversable_moves] = pos;

        for (repeat = pindex = 0; pindex < frame->reversable_moves; ++pindex)
            if (frame->position_ids [pindex] == pos)
                ++repeat;

        if (repeat >= 2)
            frame->drawn_game = POSITION_3X;
    }
    else if (!frame->in_check || generate_move_list (NULL, frame))
        frame->drawn_game = MOVES_OVER_50;

    if (!frame->white_pawns && frame->white_material < 5 &&
        !frame->black_pawns && frame->black_material < 5)
            frame->drawn_game = NO_MATE_POWER;
}

int sum_material (FRAME *frame, int color)
{
    int rank, file, sum = 0;

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            if ((SQUARE (frame, rank, file) & COLOR) == color)
                sum += piece_value [SQUARE (frame, rank, file) & PIECE];

    return sum;
}

int count_pawns (FRAME *frame, int color)
{
    int rank, file, sum = 0;

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            if ((SQUARE (frame, rank, file) & (PIECE | COLOR)) ==
                (PAWN | color))
                    ++sum;

    return sum;
}

int count_center_pawns (FRAME *frame, int color)
{
    int rank, file, sum = 0;

    for (rank = BOARD_SIDE / 2; rank <= (BOARD_SIDE + 3) / 2; ++rank)
        for (file = BOARD_SIDE / 2; file <= (BOARD_SIDE + 3) / 2; ++file)
            if ((SQUARE (frame, rank, file) & (PIECE | COLOR)) ==
                (PAWN | color))
                    ++sum;

    return sum;
}

static void print_square (FILE *out, FRAME *frame, int rank, int file);

void print_frame (FILE *out, FRAME *frame)
{
    int rank, file, nmoves = generate_move_list (NULL, frame);

    if (frame->move_color) {
        fprintf (out, "\n\n    h  g  f  e   d  c  b  a\n\n");

        for (rank = 1; rank <= BOARD_SIDE; ++rank) {

            fprintf (out, "%1d  ", rank);

            for (file = BOARD_SIDE; file; --file)
                print_square (out, frame, rank, file);

            fprintf (out, "   %1d\n", rank);
        }

        fprintf (out, "\n    h  g  f  e   d  c  b  a\n\n%d: black ",
            frame->move_number);
    }
    else {
        fprintf (out, "\n    a  b  c  d   e  f  g  h\n\n");

        for (rank = BOARD_SIDE; rank; --rank) {

            fprintf (out, "%1d  ", rank);

            for (file = 1; file <= BOARD_SIDE; ++file)
                print_square (out, frame, rank, file);

            fprintf (out, "   %1d\n", rank);
        }

        fprintf (out, "\n    a  b  c  d   e  f  g  h\n\n%d: white ",
            frame->move_number);
    }

    if (frame->in_check) {
        if (nmoves)
            fprintf (out, "is in check with %d move%s", nmoves, nmoves > 1 ? "s" : "");
        else
            fprintf (out, "is checkmated");
    }
    else {
        if (nmoves)
            fprintf (out, "has %d move%s", nmoves, nmoves > 1 ? "s" : "");
        else
            fprintf (out, "is stalemated");
    }

    if (nmoves && frame->drawn_game)
        switch (frame->drawn_game) {

            case NO_MATE_POWER:
                fprintf (out, " but neither side has sufficient material\n");
                fprintf (out, "     to checkmate, so game is drawn");
                break;

            case MOVES_OVER_50:
                fprintf (out, " but over %d moves have occurred with no\n", MAX_POS_IDS);
                fprintf (out, "     capture or pawn move, so game is drawn");
                break;

            case POSITION_3X:
                fprintf (out, " but this position has occurred three times,\n");
                fprintf (out, "     so game is drawn");
                break;
        }

    if (frame->white_material != frame->black_material) {
        int white_up = frame->white_material - frame->black_material;
        fprintf (out, " (%s up %d point%s)\n", white_up > 0 ? "white" : "black", abs (white_up), abs (white_up) > 1 ? "s" : "");
    }
    else
        fprintf (out, " (material even)\n");
}

static void print_square (FILE *out, FRAME *frame, int rank, int file)
{
    char *pnames = "  PKNBRQ";

    fputc (' ', out);

    if (SQUARE (frame, rank, file) & (PIECE | COLOR)) {

        fputc ((SQUARE (frame, rank, file) & COLOR) ? 'B' : 'W', out);
        fputc (pnames [SQUARE (frame, rank, file) & PIECE], out);
    }
    else {
        fputc (((rank + file) & 1) ? '-' : '*', out);
        fputc (((rank + file) & 1) ? '-' : '*', out);
    }
}

void print_square_name (FILE *out, int index)
{
    int rank = (index / (BOARD_SIDE + 4)) - 1;
    int file = (index % (BOARD_SIDE + 4)) - 1;

    fputc (file - 1 + 'a', out);
    fputc (rank + '0', out);
}

void print_move (FILE *out, MOVE *move)
{
    char *pnames = "  PKNBRQ";

    print_square_name (out, move->from);
    fputc ('-', out);
    print_square_name (out, move->from + move->delta);

    if (move->promo)
        fprintf (out, "/%c ", pnames [move->promo]);
    else
        fprintf (out, "   ");
}

int input_square_name (char **in)
{
    int rank, file, c;

    if (isalpha (c = *(*in)++) && (c = tolower (c)) <= 'h') {

        file = c - 'a' + 1;

        if (isdigit (c = *(*in)++) && c >= '1' && c <= '8') {

            rank = c - '0';
            return INDEX (rank, file);
        }
    }

    return FALSE;
}

int input_move (char *in, MOVE *move)
{
    int from, to;

    if ((from = input_square_name (&in)) != 0 && *in++ == '-' &&
        (to = input_square_name (&in)) != 0 && (!*in || *in == '\n')) {

            move->from = from;
            move->delta = to - from;
            move->promo = 0;
            return TRUE;
    }

    return FALSE;
}

void scramble_moves (MOVE moves [], int nmoves)
{
    int mindex, rindex;
    MOVE temp;

    for (mindex = 0; mindex < nmoves; ++mindex) {
        random_seed = ((random_seed << 4) - random_seed) ^ 1;
        rindex = (random_seed >> 1) % nmoves;
        temp = moves [rindex];
        moves [rindex] = moves [mindex];
        moves [mindex] = temp;
    }
}

int position_id (FRAME *frame)
{
    int rank, file, sum = 0;

    for (rank = 1; rank <= BOARD_SIDE; ++rank)
        for (file = 1; file <= BOARD_SIDE; ++file)
            sum += (sum << 1) + (SQUARE (frame, rank, file) & (PIECE | COLOR));

    return sum;
}

// partial Linux implementation of _kbhit()

#ifndef _WIN32

#include <sys/ioctl.h>
#include <termios.h>

int _kbhit (void)
{
    struct termios term;
    tcgetattr(0, &term);

    struct termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

int getch (void)
{
    tcflush(0, TCIFLUSH);
    return 0;
}

#endif

