#include "elements.h"

#include "../syzygy/tbprobe.h"
#include "../movegen.h"

#include <sstream>
#include <array>

namespace VariantTb {

std::string board_to_name(const Position& pos)
{
	std::stringstream white;
	std::stringstream black;
	Color lead_color = COLOR_NB;  // COLOR_NB for symmetrical positions
	for (size_t i = PIECE_TYPES.size() - 1; i > 0; i--)
	{
		auto piece_type = PIECE_TYPES[i];
		auto pieces = pos.pieces(WHITE, piece_type);
		int num_white = popcount(pieces);
		for (int j = 0; j < num_white; j++)
		{
			white << PIECE_SYMBOLS[i];
		}
		pieces = pos.pieces(BLACK, piece_type);
		int num_black = popcount(pieces);
		for (int j = 0; j < num_black; j++)
		{
			black << PIECE_SYMBOLS[i];
		}
		if (lead_color == COLOR_NB)
		{
			if (num_white > num_black)
				lead_color = WHITE;
			else if (num_black > num_white)
				lead_color = BLACK;
		}
	}
	std::string str_white = white.str();
	std::string str_black = black.str();
	if (str_white.length() > str_black.length())
		lead_color = WHITE;
	else if (str_black.length() > str_white.length())
		lead_color = BLACK;
	if (lead_color == WHITE)
	{
		white << 'v' << str_black;
		return white.str();
	}
	else
	{
		black << 'v' << str_white;
		return black.str();
	}
}

bool is_ep_position(const Position& pos)
{
	auto ep_square = pos.ep_square();
	if (ep_square == SQ_NONE)
		return false;
	Color ep_color = (pos.side_to_move() == WHITE) ? BLACK : WHITE;
	Square final_square = SQ_NONE;
	if (ep_color == WHITE)
		final_square = static_cast<Square>(ep_square + 8);
	else
		final_square = static_cast<Square>(ep_square - 8);
	if (final_square % 8 != 0)
	{
		auto mask = SquareBB[final_square - 1];
		if (pos.pieces(pos.side_to_move(), PAWN) & mask)
			return true;
	}
	if (final_square % 8 != 7)
	{
		auto mask = SquareBB[final_square + 1];
		if (pos.pieces(pos.side_to_move(), PAWN) & mask)
			return true;
	}
		return false;
}

#ifdef ATOMIC
// Game end for the side to move (atomic). Both kings are on the board in every
// table position, so these only happen after a move: a capture that explodes a
// king, or a checkmate. Stalemate is a draw.
bool is_atomic_win(const Position& pos)
{
	return pos.is_atomic_win(); // the opponent's king has exploded
}
bool is_atomic_loss(const Position& pos)
{
	if (pos.is_atomic_win())
		return false;
	return pos.is_atomic_loss() // our king has exploded
		|| (pos.checkers() && MoveList<LEGAL>(pos).size() == 0); // checkmate
}
bool is_atomic_end(const Position& pos)
{
	return is_atomic_win(pos) || is_atomic_loss(pos);
}
#else
// Whether the side to move has a legal move, without generating the move list.
// In antichess all pseudo-legal moves are legal (Position::legal()), and the move
// generator only restricts them to captures if there is one.
static bool has_any_move(const Position& pos)
{
	const Color us = pos.side_to_move();
	const Bitboard ours = pos.pieces(us), occupied = pos.pieces();
	const Bitboard pawns = pos.pieces(us, PAWN);
	const Square ep_square = pos.ep_square();
	if (ep_square != SQ_NONE && (pawn_attacks_bb(~us, ep_square) & pawns))
		return true;
	Bitboard pawn_moves = us == WHITE
		? (shift<NORTH>(pawns) & ~occupied) | (pawn_attacks_bb<WHITE>(pawns) & pos.pieces(BLACK))
		: (shift<SOUTH>(pawns) & ~occupied) | (pawn_attacks_bb<BLACK>(pawns) & pos.pieces(WHITE));
	if (pawn_moves)
		return true;
	for (Bitboard b = ours & ~pawns; b; )
	{
		Square s = pop_lsb(&b);
		if (attacks_bb(type_of(pos.piece_on(s)), s, occupied) & ~ours)
			return true;
	}
	return false;
}

bool is_anti_win(const Position& pos)
{
	assert(has_any_move(pos) == (MoveList<LEGAL>(pos).size() > 0));
	return pos.is_anti_win() || !has_any_move(pos);
}
bool is_anti_loss(const Position& pos)
{
	return pos.is_anti_loss();
}
bool is_anti_end(const Position& pos)
{
	return is_anti_win(pos) || pos.is_anti_loss();
}
#endif

} // namespace VariantTb
