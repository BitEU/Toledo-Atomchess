/*
 * Toledo Atomchess Reloaded
 * Modern C implementation for Windows Console and UNIVAC
 *
 * by Oscar Toledo Gutierrez
 * (c) Copyright 2015 Oscar Toledo Gutierrez
 *
 * Converted to C from x86 assembly
 * Supports Windows conhost and UNIVAC mainframe
 *
 * Features:
 * - Full chess movements (except promotion only to queen)
 * - Enter moves as algebraic form (D2D4) (your moves are validated)
 * - Search depth of 3-ply
 * - 0x88 board representation
 */

#ifndef TOLEDO_ATOMCHESS_H
#define TOLEDO_ATOMCHESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Platform-specific includes
#ifndef UNIVAC
#include <windows.h>
#include <conio.h>
#endif

// Board representation constants
#define BOARD_SIZE 128          // 0x88 board representation
#define BOARD_OFFSET 0          // Board starts at offset 0 in our array
#define FRONTIER 0x07           // Frontier marker value
#define EMPTY 0x00              // Empty square

// Piece constants (lower 3 bits = piece type, bit 3 = color)
#define PIECE_MASK 0x07         // Mask for piece type (without color)
#define COLOR_MASK 0x08         // Mask for color (0=black, 8=white)
#define MOVED_MASK 0x10         // Mask for "piece has moved" flag
#define PIECE_FULL_MASK 0x0F    // Mask for piece + color (without moved bit)

// Piece types (without color bit)
#define EMPTY_TYPE 0
#define PAWN 1
#define ROOK 2
#define BISHOP 3
#define QUEEN 4
#define KNIGHT 5
#define KING 6
#define FRONTIER_TYPE 7

// Colors
#define BLACK 0x00
#define WHITE 0x08

// Composite piece values
#define BLACK_PAWN (PAWN | BLACK)
#define BLACK_ROOK (ROOK | BLACK)
#define BLACK_BISHOP (BISHOP | BLACK)
#define BLACK_QUEEN (QUEEN | BLACK)
#define BLACK_KNIGHT (KNIGHT | BLACK)
#define BLACK_KING (KING | BLACK)

#define WHITE_PAWN (PAWN | WHITE)
#define WHITE_ROOK (ROOK | WHITE)
#define WHITE_BISHOP (BISHOP | WHITE)
#define WHITE_QUEEN (QUEEN | WHITE)
#define WHITE_KNIGHT (KNIGHT | WHITE)
#define WHITE_KING (KING | WHITE)

// Unmoved pieces (with moved flag cleared)
#define WHITE_ROOK_UNMOVED (WHITE_ROOK | MOVED_MASK)
#define BLACK_ROOK_UNMOVED (BLACK_ROOK | MOVED_MASK)

// Stack depth constants (adjusted for 3-ply search as noted in assembly comments)
// In C, we increment stack_depth by 2 per ply, so:
// - 1 ply = depth 2
// - 2 ply = depth 4
// - 3 ply = depth 6
#define MAX_DEPTH_PLY1 4    // Validation depth (2 plies)
#define MAX_DEPTH_PLY0 6    // Computer search depth (3 plies)

// Search score constants
#define MIN_SCORE (-32768)
#define KING_CAPTURE_SCORE 78
#define MAX_CHECKMATE_SCORE (KING_CAPTURE_SCORE * 2)
#define ILLEGAL_MOVE_SCORE (-127)

// Board dimensions for 0x88
#define BOARD_ROWS 16           // Including frontier rows
#define BOARD_VISUAL_ROWS 8     // Actual chess rows
#define BOARD_VISUAL_COLS 8     // Actual chess columns

// Piece scores (for evaluation)
extern const int piece_scores[7];

// Initial piece setup (first rank)
extern const unsigned char initial_position[8];

// Display characters for pieces
extern const char display_chars[16];

// Movement displacement tables
#define DISP_KNIGHT 0
#define DISP_KING 8
#define DISP_BISHOP 12
#define DISP_PAWN_BLACK 16
#define DISP_PAWN_WHITE 20

extern const signed char displacement[24];

// Movement offset indices
extern const unsigned char offsets[7];

// Game state structure
typedef struct {
    unsigned char board[BOARD_SIZE];    // 0x88 board representation
    int depth_limit;                     // Current depth limit for search
    int enp;                            // En passant target square (0 = none)
    int temp_score;                     // Working score during search
    int legal_move_check;               // Flag: 1=validating legal move, 0=normal play

    // Stack simulation (for recursion)
    int stack_depth;

    // Best move found (for computer)
    int best_from;
    int best_to;

    // Random seed (for move selection randomization)
    unsigned int rand_seed;
} ChessState;

// Platform-specific string copy
#ifdef UNIVAC
#define SAFE_STRCPY(dest, src, size) do { strncpy(dest, src, (size)-1); (dest)[(size)-1] = '\0'; } while(0)
#define SAFE_STRCAT(dest, src, size) strncat(dest, src, (size) - strlen(dest) - 1)
#else
#define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
#define SAFE_STRCAT(dest, src, size) strcat_s(dest, size, src)
#endif

// Function declarations

// Initialization
void init_chess(ChessState* state);
void create_board(ChessState* state);
void setup_board(ChessState* state);

// Display
void display_board(const ChessState* state);
void display_char(char c);

// Input
int read_key(void);
int key_to_coord(void);

// Move generation and validation
int play(ChessState* state, int origin, int target, int current_color, int* best_score);
int play_validate(ChessState* state, int origin, int target, int current_color);
int is_legal_move(ChessState* state, int from, int to, int color);

// Move execution
void make_move(ChessState* state, int from, int to);
void unmake_move(ChessState* state, int from, int to, unsigned char captured);

// Special moves
int is_en_passant(int from, int to, int diff);
void handle_castling(ChessState* state, int from, int to, int piece);
void handle_promotion(ChessState* state, int to, unsigned char* piece_ptr);

// Board utilities
int get_square(const ChessState* state, int pos);
void set_square(ChessState* state, int pos, unsigned char value);
int is_valid_square(int pos);
int get_piece_type(unsigned char piece);
int get_piece_color(unsigned char piece);
void position_to_algebraic(int pos, char* output);

// AI/Search
void computer_move(ChessState* state, int color);
int evaluate_position(const ChessState* state, int color);

// Random number (for move selection)
unsigned char get_random_byte(ChessState* state);

// Main game loop
void run_game(ChessState* state);

// Platform-specific functions
#ifndef UNIVAC
void console_setup(void);
#endif

#endif // TOLEDO_ATOMCHESS_H
