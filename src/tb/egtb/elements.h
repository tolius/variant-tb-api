#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

#include "../position.h"

#include <string>
#include <array>
#include <sstream>

namespace VariantTb {

constexpr std::string_view PIECE_SYMBOLS = " PNBRQK";
static constexpr std::array<PieceType, 7> PIECE_TYPES = { NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

// Variant of the build: ANTI (antichess, .an0 files) or ATOMIC (.at0 files)
#if defined(ANTI) == defined(ATOMIC)
#error "define exactly one of ANTI and ATOMIC"
#endif
#ifdef ATOMIC
constexpr Variant TB_VARIANT = ATOMIC_VARIANT;
#else
constexpr Variant TB_VARIANT = ANTI_VARIANT;
#endif


std::string board_to_name(const Position& pos);
bool is_ep_position(const Position& pos);

#ifdef ATOMIC
// Game end for the side to move: win = the opponent's king has exploded,
// loss = our king has exploded or we are checkmated
bool is_atomic_win(const Position& pos);
bool is_atomic_loss(const Position& pos);
bool is_atomic_end(const Position& pos);
#else
bool is_anti_win(const Position& pos);
bool is_anti_loss(const Position& pos);
bool is_anti_end(const Position& pos);
#endif

// Game end for the side to move in the variant of the build
#ifdef ATOMIC
inline bool is_game_win(const Position& pos) { return is_atomic_win(pos); }
inline bool is_game_loss(const Position& pos) { return is_atomic_loss(pos); }
inline bool is_game_end(const Position& pos) { return is_atomic_end(pos); }
// Only the two kings are left (after an explosion): a draw, there is no KvK table
inline bool is_bare_kings(const Position& pos) { return pos.count<ALL_PIECES>() == 2; }
#else
inline bool is_game_win(const Position& pos) { return is_anti_win(pos); }
inline bool is_game_loss(const Position& pos) { return is_anti_loss(pos); }
inline bool is_game_end(const Position& pos) { return is_anti_end(pos); }
inline bool is_bare_kings(const Position&) { return false; } // antichess has a KvK table
#endif

struct separate_thousands : std::numpunct<char>
{
	char_type do_thousands_sep() const override { return ','; }  // separate with commas
	string_type do_grouping() const override { return "\3"; } // groups of 3 digit
};

} // namespace VariantTb

#endif
