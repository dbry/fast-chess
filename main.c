////////////////////////////////////////////////////////////////////////////
//                         **** FAST-CHESS ****                           //
//                     Trivial Chess Playing Program                      //
//                    Copyright (c) 2020 David Bryant                     //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// main.c

// This is a simple chess-playing program that I wrote a lifetime ago, just
// because I thought I could. The idea was if I could make the chess engine
// really fast by using a lot of inline code and making the final evaluation
// as simple as possible, I could process enough moves to make a really good
// player, but it never really worked out.

// I don't really know how good it is relative to other really simple chess
// programs, but at level 5 (which is tolerable) it provides me a challenge
// (which isn't saying much) and it doesn't seem to make really bad moves.

// It has no stored openings and it gets pretty lost in the endgame because
// it can't look far ahead enough to force a checkmate even with overwhelming
// material or recognize passed pawns until it's too late. So even if you
// screw up and lose in the middle game you can sometimes get a pawn promoted
// when it's not paying attention, or at least squeak out a draw by keeping
// your king in the center of the board.

// Another thing I wanted was for the program to not play the same game every
// time, so it randomly scrambles the moves before evaluating them (which
// results in different outcomes). This also makes it more interesting when
// it plays itself (which is a feature I've used extensively in development).

// To its credit it does handle all legal chess, including castling and its
// rules, en passant capture, and detecting draws based on the 50-move and
// 3-time repeated position rules.

// Good luck!

#include "fast-chess.h"

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
int _kbhit (void), getch (void);
#endif

static void print_frame (FILE *out, FRAME *frame);
static void print_square (FILE *out, FRAME *frame, int rank, int file);
static void print_square_name (FILE *out, int index);
static void print_move (FILE *out, MOVE *move);
static void print_game (FILE *out, MOVE *gameplay, int nmoves);
static int input_square_name (char **in);
static int input_move (char *in, MOVE *move);
static int input_game (FILE *in, MOVE **gameplay, int *gameplay_moves);

static const char *sign_on = "\n"
" FAST-CHESS  Trivial Chess Playing Program  Version 0.2\n"
" Copyright (c) 2020 David Bryant.  All Rights Reserved.\n\n";

static char *help = "\n\
 Usage:   fast-chess [options] [saved game to load on startup]\n\n\
 Options:\n\
  -H:     display this help message\n\
  -R:     randomize for different games\n\
  -Tn:    maximum thread count, 0 or 1 for single-threaded\n\
  -Gn:    specify number of games to play (otherwise stops on keypress)\n\
  -Wn:    computer plays white at level n (1 to about 6; higher is slower)\n\
  -Bn:    computer plays black at level n (1 to about 6; higher is slower)\n\n\
 Commands:\n\
  H <cr>:        display this help message\n\
  W n <cr>:      computer plays white at level n\n\
  B n <cr>:      computer plays black at level n\n\
  E n <cr>:      evaluate legal moves at level n (default=1)\n\
  T n <cr>:      take back n moves (default=1)\n\
  W <cr>:        returns white play to user\n\
  B <cr>:        returns black play to user\n\
  S <file><cr>:  save game to specified file\n\
  L <file><cr>:  load game from specified file\n\
  R <cr>:        resign game and start new game\n\
  Q <cr>:        resign game and quit\n\n";

int main (argc, argv) int argc; char **argv;
{
    int nmoves, mindex, maxmoves = 0, minmoves = 1000, asked4help = FALSE, quit = FALSE, resign = FALSE, max_threads;
    int games_to_play = 0, games = 0, whitewins = 0, blackwins = 0, draws = 0, whitedraws = 0, blackdraws = 0;
    int default_flags = EVAL_POSITION | EVAL_SCALE | EVAL_PRUNE | EVAL_DECAY | EVAL_SCRAMBLE;
    int white_level = 0, black_level = 0, level;
    time_t start_time, stop_time;
    MOVE moves [MAX_MOVES + 10];
    char *init_filename = NULL;
    long totalmoves = 0;
    FRAME frame;
    FILE *file;

#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    max_threads = sysinfo.dwNumberOfProcessors;
#else
    max_threads = sysconf (_SC_NPROCESSORS_ONLN);
#endif

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
                    init_random (time (NULL));
                    srand (time (NULL));
                    break;

                case 'T': case 't':
                    max_threads = atoi (++*argv);
                    break;

                default:
                    fprintf (stderr, "illegal option: %s\n%s", --*argv, help);
                    exit (1);
            }
        else if (init_filename) {
            fprintf (stderr, "argument ignored: %s\n%s", *argv, help);
            exit (1);
        }
        else
            init_filename = *argv;
    }

    printf ("%s", sign_on);

    if (asked4help)
        printf ("%s", help);

    time (&start_time);

    while (!quit && (!white_level || !black_level || !_kbhit())) {
        int gameplay_moves = 0;
        MOVE *gameplay = NULL;

        init_frame (&frame);

        if (init_filename) {
            FILE *file = fopen (init_filename, "rt");

            if (file) {
                if (input_game (file, &gameplay, &gameplay_moves))
                    for (mindex = 0; mindex < gameplay_moves; ++mindex)
                        execute_move (&frame, gameplay + mindex);

                if (frame.move_number == 1 && !frame.move_color)
                    fprintf (stderr, "\ninvalid game file %s\n\007", init_filename);

                fclose (file);
            }
            else
                fprintf (stderr, "\ncan't open file %s\n\007", init_filename);

            init_filename = NULL;
        }

        while (!frame.drawn_game) {
            MOVE bestmove;

            level = (frame.move_color) ? black_level : white_level;
            bestmove.from = 0;

            if (level > 0) {
                frame.depth = level;
                frame.flags = default_flags;
                frame.max_threads = max_threads;
                frame.bestmove_p = &bestmove;
                eval_position (&frame);
            }
            else if ((nmoves = generate_move_list (moves, &frame)) != 0) {
                if (nmoves > MAX_MOVES) {
                    print_frame (stdout, &frame);
                    fprintf (stderr, "%d legal moves!\n", nmoves);
                    exit (1);
                }

                if (!level) {
                    char command [81], *cptr;

                    print_frame (stdout, &frame);
                    fprintf (stderr, "input move or command: ");
                    quit = resign = FALSE;

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
                        int eval_level, take_back = 0;;

                        while (*++cptr && *cptr == ' ');

                        switch (command [0]) {

                            default:
                                fprintf (stderr, "\nillegal command\n\007");

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

                            case 'T': case 't':
                                take_back = atoi (cptr);

                                if (!take_back) take_back = 1;

                                if (white_level || black_level)
                                    take_back *= 2;

                                if (take_back > gameplay_moves)
                                    take_back = gameplay_moves;

                                if (take_back) {
                                    init_frame (&frame);
                                    gameplay_moves -= take_back;

                                    for (mindex = 0; mindex < gameplay_moves; ++mindex)
                                        execute_move (&frame, gameplay + mindex);
                                }
                                else
                                    fprintf (stderr, "\nno moves to take back!\n\007");

                                break;

                            case 'E': case 'e':
                                eval_level = atoi (cptr);
                                if (eval_level < 1) eval_level = 1;

                                printf ("\n");

                                for (mindex = 0; mindex < nmoves && !_kbhit (); ++mindex) {
                                    int depth;

                                    printf ("%2d: ", mindex + 1);
                                    print_move (stdout, moves + mindex);
                                    printf ("score%s =", eval_level > 1 ? "s" : "");

                                    for (depth = 0; depth < eval_level && !_kbhit (); ++depth) {
                                        FRAME temp = frame;

                                        temp.depth = depth;
                                        temp.bestmove_p = NULL;
                                        temp.flags = default_flags;
                                        temp.max_threads = max_threads;
                                        execute_move (&temp, moves + mindex);
                                        printf ("%7d", - (int) (long) eval_position (&temp));
                                        fflush (stdout);
                                    }

                                    printf ("\n");
                                }

                                if (_kbhit ())
                                    getch ();

                                break;

                            case 'S': case 's':
                                if (!*cptr) {
                                    fprintf (stderr, "\nneed filename\n\007");
                                    break;
                                }

                                if ((file = fopen (cptr, "wt")) == NULL) {
                                    fprintf (stderr, "\ncan't open file %s\n\007", cptr);
                                    break;
                                }

                                print_game (file, gameplay, gameplay_moves);
                                fclose (file);
                                break;

                            case 'L': case 'l':
                                if (!*cptr) {
                                    fprintf (stderr, "\nneed filename\n\007");
                                    break;
                                }

                                if ((file = fopen (cptr, "rt")) == NULL) {
                                    fprintf (stderr, "\ncan't open file %s\n\007", cptr);
                                    break;
                                }

                                if (!input_game (file, &gameplay, &gameplay_moves)) {
                                    fprintf (stderr, "\ninvalid game file %s\n\007", cptr);
                                    break;
                                }

                                fclose (file);
                                init_frame (&frame);

                                for (mindex = 0; mindex < gameplay_moves; ++mindex)
                                    execute_move (&frame, gameplay + mindex);

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
                gameplay = realloc (gameplay, (gameplay_moves + 1) * sizeof (MOVE));
                gameplay [gameplay_moves++] = bestmove;
            }
            else
                break;
        }

        free (gameplay);
        gameplay = NULL;

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

static void print_frame (FILE *out, FRAME *frame)
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

static void print_square_name (FILE *out, int index)
{
    int rank = (index / (BOARD_SIDE + 4)) - 1;
    int file = (index % (BOARD_SIDE + 4)) - 1;

    fputc (file - 1 + 'a', out);
    fputc (rank + '0', out);
}

static void print_move (FILE *out, MOVE *move)
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

static void print_game (FILE *out, MOVE *gameplay, int nmoves)
{
    int mindex;

    for (mindex = 0; mindex < nmoves; ++mindex) {
        if (!(mindex & 1))
            fprintf (out, "%3d: ", (mindex >> 1) + 1);

        print_move (out, gameplay + mindex);

        if ((mindex & 1) || mindex == nmoves - 1)
            fprintf (out, "\n");
    }
}

static int input_square_name (char **in)
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

static int input_move (char *in, MOVE *move)
{
    int from, to;

    if ((from = input_square_name (&in)) != 0 && *in++ == '-' &&
        (to = input_square_name (&in)) != 0 && (!*in || *in == '\n' || *in == '/')) {

            move->from = from;
            move->delta = to - from;
            move->promo = 0;

            if (*in == '/') {
                if (in [1] == 'B' || in [1] == 'b')
                    move->promo = BISHOP;
                else if (in [1] == 'N' || in [1] == 'n')
                    move->promo = KNIGHT;
                else if (in [1] == 'Q' || in [1] == 'q')
                    move->promo = QUEEN;
                else if (in [1] == 'R' || in [1] == 'r')
                    move->promo = ROOK;
                else
                    return FALSE;
            }

            return TRUE;
    }

    return FALSE;
}

static int input_game (FILE *in, MOVE **gameplay, int *gameplay_moves)
{
    char white [16], black [16];
    MOVE *input_moves = NULL;
    int input_count = 0;

    while (1) {
        int num, count = fscanf (in, "%d:%10s%10s", &num, white, black);

        if (count < 2)
            break;

        if (num != input_count / 2 + 1) {
            free (input_moves);
            return FALSE;
        }

        input_moves = realloc (input_moves, sizeof (MOVE) * (input_count + 1));

        if (!input_move (white, input_moves + input_count++)) {
            free (input_moves);
            return FALSE;
        }

        if (count == 3) {
            input_moves = realloc (input_moves, sizeof (MOVE) * (input_count + 1));

            if (!input_move (black, input_moves + input_count++)) {
                free (input_moves);
                return FALSE;
            }
        }
    }

    if (!input_count) {
        free (input_moves);
        return FALSE;
    }

    free (*gameplay);
    *gameplay = input_moves;
    *gameplay_moves = input_count;
    return TRUE;
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

