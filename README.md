## FAST-CHESS

Trivial Chess Playing Program

Copyright (c) 2020 David Bryant.

All Rights Reserved.

Distributed under the [BSD Software License](https://github.com/dbry/fast-chess/blob/master/license.txt).

## What is this?

This is a simple chess-playing program that I wrote a lifetime ago, just because I thought I could. The idea was if I could make the chess engine really fast by using a lot of inline code, making the final evaluation as simple as possible, and using multithreading (just added), I could process enough moves to make a really good player.

I don't really know how good it is relative to other really simple chess programs, but at level 5 (which is very tolerable speed-wise) it provides me a strong challenge and it never seems to make obviously bad moves.

It has no stored openings and it gets pretty lost in the endgame because it can't look far ahead enough to force a checkmate even with overwhelming material or recognize passed pawns until it's too late. So even if you screw up and lose in the middle game you can sometimes get a pawn promoted when it's not paying attention, or at least squeak out a draw by keeping your king in the center of the board.

Another thing I wanted was for the program to not play the same game every time, so it randomly scrambles the moves before evaluating them (which results in different outcomes). This also makes it more interesting when it plays itself (which is a feature I've used extensively in development).

To its credit it handles all legal chess, including castling and its rules, en passant capture, and detecting draws based on the 50-move and 3-time repeated position rules.

For version 0.2 I added the ability to take back moves and added multithreaded operation via pthreads to speed up play on multicore machines.

Good luck!

## Building

FAST-CHESS is a command-line application. To build it on Linux:

> $ gcc -O3 *.c -pthread -o fast-chess

There are also executables for Windows and Mac available on the [release page](https://github.com/dbry/fast-chess/releases/tag/v0.2).

Here's the "help" display and the board display format:

```
 FAST-CHESS  Trivial Chess Playing Program  Version 0.2
 Copyright (c) 2020 David Bryant.  All Rights Reserved.

 Usage:   fast-chess [options] [saved game to load on startup]

 Options:
  -H:     display this help message
  -R:     randomize for different games
  -Tn:    maximum thread count, 0 or 1 for single-threaded
  -Gn:    specify number of games to play (otherwise stops on keypress)
  -Wn:    computer plays white at level n (1 to about 6; higher is slower)
  -Bn:    computer plays black at level n (1 to about 6; higher is slower)

 Commands:
  H <cr>:        display this help message
  W n <cr>:      computer plays white at level n
  B n <cr>:      computer plays black at level n
  E n <cr>:      evaluate legal moves at level n (default=1)
  T n <cr>:      take back n moves (default=1)
  W <cr>:        returns white play to user
  B <cr>:        returns black play to user
  S <file><cr>:  save game to specified file
  L <file><cr>:  load game from specified file
  R <cr>:        resign game and start new game
  Q <cr>:        resign game and quit

    a  b  c  d   e  f  g  h

8   BR BN BB BQ BK BB BN BR   8
7   BP BP BP BP BP BP BP BP   7
6   -- ** -- ** -- ** -- **   6
5   ** -- ** -- ** -- ** --   5
4   -- ** -- ** -- ** -- **   4
3   ** -- ** -- ** -- ** --   3
2   WP WP WP WP WP WP WP WP   2
1   WR WN WB WQ WK WB WN WR   1

    a  b  c  d   e  f  g  h

1: white has 20 moves (material even)
input move or command:

```
## Future improvements?

There are many ways to improve fast-chess by adding stuff, but the first thing would be to determine if there are any simple tweaks or fixes to improve it easily. After that, here are some ideas for future development:

1. Incorporating book openings would help play against advanced players.

2. The position evaluation is based on simply the material of both sides, the number of moves each side can make (a good indication of development), and a little weighing for having pawns in the center. A minor issue with this is that it promotes early queen development, but a worse problem is that it is hopeless for endgame scenarios and recognizing passed pawns. A scheme that modifies the position evaluation based on the distribution of material would help here.

3. ~~Using multiple cores for evaluation would help with modern CPUs. This wouldn't be that hard to do, but is complicated by the alpha-beta pruning.~~ Done.

4. I believe that more advanced chess engines use hashes of the evaluated positions to avoid duplication. This program uses a trivial hash to implement the 3-time position repeat draw, but something more might be needed to reduce collisions.

5. Thinking when it's the player's move.
