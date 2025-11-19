/*
 * Toledo Atomchess Reloaded
 * Main implementation
 *
 * by Oscar Toledo Gutierrez
 * (c) Copyright 2015 Oscar Toledo Gutierrez
 *
 * Converted to C from x86 assembly
 */

#include "toledo_atomchess.h"

// Global data arrays (matching assembly DATA sections)

// Piece scores for evaluation (line 416-417)
const int piece_scores[7] = {
    0,  // Empty
    1,  // Pawn
    5,  // Rook
    3,  // Bishop
    9,  // Queen
    3,  // Knight
    0   // King (special handling)
};

// Initial position setup (first rank) - line 414-415
const unsigned char initial_position[8] = {
    0x12, 0x15, 0x13, 0x14, 0x16, 0x13, 0x15, 0x12
    // Rook, Knight, Bishop, Queen, King, Bishop, Knight, Rook
};

// Display characters for pieces (two-character format for teletype)
// Index 0-7: Black pieces, Index 8-15: White pieces
const char display_chars[16][3] = {
    "..", "BP", "BR", "BB", "BQ", "BN", "BK", "??",  // Black: empty, pawn, rook, bishop, queen, knight, king, frontier
    "..", "WP", "WR", "WB", "WQ", "WN", "WK", "??"   // White: empty, pawn, rook, bishop, queen, knight, king, frontier
};

// Movement displacement table (lines 427-432)
// Knight, King, Bishop, Pawn-black, Pawn-white movements
const signed char displacement[24] = {
    // Knight moves (8 directions)
    -33, -31, -18, -14, 14, 18, 31, 33,
    // King/Queen/Rook moves (4 cardinal directions)
    -16, 16, -1, 1,
    // Bishop/Queen moves (4 diagonal directions)
    15, 17, -15, -17,
    // Black pawn moves (advance, double-advance, capture-left, capture-right)
    -17, -15, -16, -32,
    // White pawn moves (advance, double-advance, capture-left, capture-right)
    15, 17, 16, 32
};

// Movement offset indices (lines 419-426)
const unsigned char offsets[7] = {
    0,  // Empty (unused)
    16, // Pawn (index 16 in displacement, adjusted by color)
    8,  // Rook
    12, // Bishop
    8,  // Queen
    0,  // Knight
    8   // King
};

// Platform-specific console setup
#ifndef UNIVAC
void console_setup(void) {
    // Set console to UTF-8 for better text handling
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#endif

// Random number generator (using time-based seed + simple LCG)
unsigned char get_random_byte(ChessState* state) {
    // Simple LCG: multiply by prime, add offset
    state->rand_seed = (state->rand_seed * 1103515245 + 12345) & 0x7fffffff;
    return (unsigned char)(state->rand_seed & 0xFF);
}

// Main entry point
int main(void) {
#ifndef UNIVAC
    console_setup();
#endif

    ChessState state;

    // Initialize BSS (zero out all state)
    memset(&state, 0, sizeof(ChessState));

    // Initialize random seed
    state.rand_seed = (unsigned int)time(NULL);

    init_chess(&state);
    run_game(&state);

    return 0;
}

// Initialize chess game (lines 62-83)
void init_chess(ChessState* state) {
    create_board(state);
    setup_board(state);
}

// Create empty board with frontier markers (lines 62-71)
void create_board(ChessState* state) {
    // Initialize entire board array to empty first
    for (int i = 0; i < BOARD_SIZE; i++) {
        state->board[i] = EMPTY;
    }

    // Now set frontier markers for off-board squares
    for (int i = 0; i < BOARD_SIZE; i++) {
        if ((i & 0x88) != 0) {
            state->board[i] = FRONTIER;
        }
    }
}

// Setup initial piece positions (lines 72-83)
void setup_board(ChessState* state) {
    // Reset en passant state
    state->enp = 0;

    // Place pieces for both sides (matching assembly exactly)
    for (int i = 0; i < 8; i++) {
        unsigned char piece = initial_position[i];

        // Black pieces on first rank (row 0)
        state->board[i] = piece;

        // White pieces on eighth rank (row 7) - piece | 0x08 for white
        state->board[i + 0x70] = piece | 0x08;

        // Black pawns on second rank (row 1) - 0x11 = black pawn
        state->board[(i + 1) + 0x0F] = 0x11;

        // White pawns on seventh rank (row 6) - 0x19 = white pawn
        state->board[(i + 1) + 0x5F] = 0x19;
    }
}

// Display the board (lines 273-288)
void display_board(const ChessState* state) {
    // Display column labels (uppercase for teletype)
    printf("    A   B   C   D   E   F   G   H\n\n");

    // Display 8 rows of 8 squares each
    for (int row = 0; row < 8; row++) {
        int row_base = row * 16;  // 0x88 board: each row is 16 bytes apart
        int rank = 8 - row;       // Chess rank (8 to 1)

        // Display rank number
        printf("%d  ", rank);

        for (int col = 0; col < 8; col++) {
            int pos = row_base + col;
            unsigned char piece = state->board[pos] & PIECE_FULL_MASK;  // Remove "moved" bit
            printf("%s", display_chars[piece]);

            // Add spacing between pieces (but not after the last piece)
            if (col < 7) {
                printf("  ");
            }
        }

        printf("\n");
    }
}

// Display a single character (lines 405-412)
void display_char(char c) {
#ifdef UNIVAC
    putchar(c);
#else
    putchar(c);
#endif
}

// Read a key from keyboard (lines 403-412)
int read_key(void) {
#ifdef UNIVAC
    return getchar() & 0x0F;  // Get character and extract column
#else
    // Use _getch() on Windows to get character without echo
    int ch = _getch();

    // Echo the character
    putchar(ch);

    return ch & 0x0F;
#endif
}

// Read algebraic coordinate (lines 290-297)
// Returns board position from algebraic notation (e.g., 'D' '4')
int key_to_coord(void) {
    int letter = read_key();
    int di = letter + 127;  // Calculate board column (board + 127)

    int digit = read_key();
    int offset = (digit << 4);  // Multiply digit row by 16
    di -= offset;  // Subtract to get final position

    return di;
}

// Check if position is valid on 0x88 board
int is_valid_square(int pos) {
    return (pos & 0x88) == 0;
}

// Get piece at square
int get_square(const ChessState* state, int pos) {
    if (!is_valid_square(pos) || pos < 0 || pos >= BOARD_SIZE) {
        return FRONTIER;
    }
    return state->board[pos];
}

// Set piece at square
void set_square(ChessState* state, int pos, unsigned char value) {
    if (is_valid_square(pos) && pos >= 0 && pos < BOARD_SIZE) {
        state->board[pos] = value;
    }
}

// Get piece type (without color/moved bits)
int get_piece_type(unsigned char piece) {
    return piece & PIECE_MASK;
}

// Get piece color
int get_piece_color(unsigned char piece) {
    return piece & COLOR_MASK;
}

// Convert board position to algebraic notation (e.g., 0x63 -> "D2")
void position_to_algebraic(int pos, char* output) {
    int col = pos & 0x07;           // Column 0-7
    int row = (pos >> 4);           // Row 0-7 in 0x88 board
    int rank = 8 - row;             // Convert to chess rank (1-8)

    output[0] = 'A' + col;          // Column letter A-H
    output[1] = '0' + rank;         // Rank digit 1-8
    output[2] = '\0';
}

// Test if move is en passant (lines 260-271)
int is_en_passant(int from, int to, int diff) {
    (void)to;  // Unused in this simplified version

    // Check if diagonal move (bit 0 of diff is set)
    if ((diff & 1) == 0) {
        return 0;  // Not diagonal
    }

    // Check if target square is empty (captured piece check done elsewhere)
    // Returns 1 if en passant conditions met

    // Calculate en passant capture square
    int ep_square;
    if (diff & 2) {  // Going left?
        ep_square = from - 1;
    } else {
        ep_square = from + 1;
    }

    return ep_square;
}

// Main play/search function (lines 111-400)
// This is the heart of the chess engine - recursive minimax search
int play(ChessState* state, int origin_hint, int target_hint, int current_color, int* best_score) {
    int bp = MIN_SCORE;  // Current best score for this position
    int saved_enp = state->enp;  // Save en passant state for restoration

    // Iterate through all squares looking for pieces to move
    for (int si = 0; si < 120; si++) {
        // Skip invalid squares (0x88 board boundary check)
        if ((si & 0x88) != 0) {
            continue;
        }
        unsigned char piece_at_origin = state->board[si];
        unsigned char piece_type = (piece_at_origin ^ current_color) & PIECE_FULL_MASK;

        // Translate piece type to 0-5 range (check for valid piece)
        int piece_idx = piece_type - 1;

        // Skip if not our piece (empty=-1, enemy=8-13, frontier=6)
        if (piece_idx < 0 || piece_idx >= 6) {
            continue;
        }

        // Determine piece movement characteristics
        int movement_offset;
        int movement_count;

        // Special handling for pawns based on color
        if (piece_idx == 0) {  // Pawn
            movement_count = 4;  // Pawns have 4 movement types
            if (current_color == BLACK) {
                movement_offset = 16;  // Black pawn moves: displacement[16-19]
            } else {
                movement_offset = 20;  // White pawn moves: displacement[20-23]
            }
            piece_idx = 4;  // Adjusted index for later logic
        } else {
            // Non-pawn pieces
            piece_idx++;  // Adjust for lookup (rook=2 becomes 3, etc.)
            piece_idx += 4;  // Further adjustment
            movement_count = piece_idx & 0x0C;  // Total movements
            movement_offset = offsets[piece_idx - 4];
        }

        // Determine if this piece slides (rook/bishop/queen) or jumps (knight/king/pawn)
        int is_sliding_piece = (piece_idx >= 6 && piece_idx <= 8);  // Rook(6), Bishop(7), Queen(8) after adjustments

        // Try each movement direction for this piece
        for (int move_dir = 0; move_dir < movement_count; move_dir++) {
            int di = si;  // Start from origin square

            // Follow this direction until blocked or edge
            for (;;) {
                // Calculate next target square
                int disp_idx = movement_offset + move_dir;
                if (disp_idx >= 24) disp_idx = 16;  // Wrap around for pawn

                di += displacement[disp_idx];

                // Bounds check - skip if off board (0x88 check)
                if ((di & 0x88) != 0 || di < 0 || di >= BOARD_SIZE) {
                    break;  // Off board, try next direction
                }

                unsigned char target_piece = state->board[di];
                unsigned char target_type = target_piece & PIECE_FULL_MASK;

                // Check if target is empty
                if (target_type == EMPTY) {
                    // Special pawn logic: can't capture on straight move
                    if (movement_offset >= 16 && movement_count < 3) {
                        // Pawn moving straight - this is OK
                        goto valid_move;
                    } else if (movement_offset >= 16) {
                        // Pawn trying to capture empty square - invalid unless en passant
                        if (di == state->enp) {
                            // En passant capture
                            goto valid_move;
                        }
                        break;  // Invalid diagonal pawn move to empty square
                    }

                    goto valid_move;
                } else {
                    // Square is occupied
                    // Check if it's a valid capture
                    int target_color = target_piece & COLOR_MASK;

                    if (target_color != current_color && (target_type & PIECE_MASK) < 7) {
                        // Enemy piece - valid capture
                        goto valid_move;
                    } else {
                        // Own piece or frontier
                        break;
                    }
                }

                continue;

valid_move:
                // This is a potentially valid move

                // Check for king capture (checkmate)
                int captured_type = get_piece_type(target_piece);
                if (captured_type == KING) {
                    *best_score = KING_CAPTURE_SCORE;
                    if (state->stack_depth > MAX_DEPTH_PLY1) {
                        *best_score = MAX_CHECKMATE_SCORE;
                    }
                    return 1;  // King captured!
                }

                // Legal move validation (assembly lines 242-256)
                // If we're checking a specific move and at root level, check if it matches
                if (state->legal_move_check && state->stack_depth == 0) {
                    if (si == origin_hint && di == target_hint) {
                        // This is the move we're validating - it's legal!
                        *best_score = 0;  // Return 0 to indicate success
                        return 0;
                    }
                    // Not the move we're looking for, skip evaluation
                    continue;
                }

                // Make the move
                unsigned char saved_target_piece = state->board[di];
                unsigned char saved_origin_piece = state->board[si];

                state->board[di] = piece_at_origin & PIECE_FULL_MASK;  // Move piece, clear moved bit
                state->board[si] = EMPTY;

                // Recursive search if not at depth limit
                int move_score = piece_scores[captured_type];

                if (state->stack_depth < state->depth_limit) {
                    int sub_score = 0;
                    state->stack_depth += 2;
                    play(state, -1, -1, current_color ^ COLOR_MASK, &sub_score);
                    state->stack_depth -= 2;
                    move_score -= sub_score;
                }

                // Unmake the move
                state->board[si] = saved_origin_piece;
                state->board[di] = saved_target_piece;

                // Check if this is the best move so far
                if (move_score > bp) {
                    bp = move_score;

                    // Save best move at root level
                    if (state->stack_depth == 0) {
                        state->best_from = si;
                        state->best_to = di;
                    }
                }

                // For non-sliding pieces (knight, king, pawn), stop after first square
                // For sliding pieces (rook, bishop, queen), continue until blocked
                if (!is_sliding_piece || target_type != EMPTY) {
                    break;  // Non-slider, or blocked - try next direction
                }
            }
        }
    }

    // If we're validating a legal move and didn't find it, return illegal score
    if (state->legal_move_check && state->stack_depth == 0) {
        *best_score = ILLEGAL_MOVE_SCORE;
    } else {
        *best_score = bp;
    }

    state->enp = saved_enp;  // Restore en passant state
    return 0;
}

// Validate and execute player move (lines 108-110)
int play_validate(ChessState* state, int origin, int target, int current_color) {
    state->legal_move_check = 1;
    state->depth_limit = MAX_DEPTH_PLY1;
    state->stack_depth = 0;

    int score = 0;
    play(state, origin, target, current_color, &score);

    // Check if move was legal (score >= ILLEGAL_MOVE_SCORE)
    return score;
}

// Execute computer move (lines 99-103)
void computer_move(ChessState* state, int color) {
    state->legal_move_check = 0;
    state->depth_limit = MAX_DEPTH_PLY0;
    state->stack_depth = 0;

    state->best_from = -1;
    state->best_to = -1;

    int score = 0;
    play(state, -1, -1, color, &score);

    // Execute the best move found and display it
    if (state->best_from >= 0 && state->best_to >= 0) {
        char from_str[3], to_str[3];
        position_to_algebraic(state->best_from, from_str);
        position_to_algebraic(state->best_to, to_str);
        printf("%s%s\n", from_str, to_str);
        make_move(state, state->best_from, state->best_to);
    }
}

// Make a move on the board
void make_move(ChessState* state, int from, int to) {
    unsigned char piece = state->board[from];
    unsigned char captured = state->board[to];

    // Clear moved bit when moving
    state->board[to] = piece & PIECE_FULL_MASK;
    state->board[from] = EMPTY;

    // Handle special moves (castling, en passant, promotion)
    int piece_type = get_piece_type(piece);

    // Check for pawn promotion
    if (piece_type == PAWN) {
        if ((to & 0xF0) == 0x00 || (to & 0xF0) == 0x70) {
            // Promote to queen
            state->board[to] = (piece & COLOR_MASK) | QUEEN;
        }

        // Check for en passant capture
        int diff = to - from;
        if ((diff & 0x0F) > 1 || (diff & 0x0F) < -1) {
            // Diagonal move
            if (captured == EMPTY && to == state->enp) {
                // En passant: remove captured pawn
                int ep_pawn_square;
                if (diff > 0) {
                    ep_pawn_square = to - 16;  // White captured black
                } else {
                    ep_pawn_square = to + 16;  // Black captured white
                }
                state->board[ep_pawn_square] = EMPTY;
            }
        }

        // Set en passant target if pawn moved two squares
        if (diff == 32 || diff == -32) {
            state->enp = to;
        } else {
            state->enp = 0;
        }
    } else {
        state->enp = 0;
    }

    // Handle castling
    if (piece_type == KING) {
        int diff = to - from;
        if (diff == 2) {
            // Castle kingside
            state->board[to - 1] = state->board[to + 1];
            state->board[to + 1] = EMPTY;
        } else if (diff == -2) {
            // Castle queenside
            state->board[to + 1] = state->board[to - 2];
            state->board[to - 2] = EMPTY;
        }
    }
}

// Main game loop (lines 88-103)
void run_game(ChessState* state) {
    while (1) {
        // Display board
        display_board(state);

        // Get player move
        printf("\nYour move (e.g., D2D4 or 'quit' to exit): ");

        // Read first character WITHOUT masking to check for quit command
#ifdef UNIVAC
        int first_char_raw = getchar();
#else
        int first_char_raw = _getch();
        putchar(first_char_raw);  // Echo the character
#endif
        char first_upper = (char)toupper(first_char_raw);

        // Check for 'Q' (quit)
        if (first_upper == 'Q') {
            printf("\nThanks for playing!\n");
            exit(0);
        }

        // Apply masking for chess coordinate processing
        int from_col = first_char_raw & 0x0F;
        int from_row = read_key();
        int to_col = read_key();
        int to_row = read_key();

        // Calculate board positions from coordinates
        int from = (from_col + 127) - (from_row << 4);
        int to = (to_col + 127) - (to_row << 4);
        printf("\n");

        // Validate player move (WHITE)
        int score = play_validate(state, from, to, WHITE);

        if (score < ILLEGAL_MOVE_SCORE) {
            printf("Illegal move! Try again.\n");
            continue;
        }

        // Execute player move
        make_move(state, from, to);

        // Display board after player move
        display_board(state);
        printf("\nComputer thinking...");

        // Computer move (BLACK)
        computer_move(state, BLACK);
    }
}
