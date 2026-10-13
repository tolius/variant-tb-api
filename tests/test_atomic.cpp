#include "atomic_tb_api.h"

#include "tb/types.h"
#include "tb/thread.h"
#include "tb/movegen.h"
#include "tb/egtb/tb_reader.h"
#include "crc_check.h"

#include <algorithm>
#include <tuple>
#include <cctype>
#include <locale>
#include <iostream>
#include <iomanip>
#include <charconv>
#include <deque>
#include <sstream>


using namespace VariantTb;


constexpr size_t MAX_CARDINALITY = 5;

int probe(const Position& pos, int& dtw)
{
	int squares[COLOR_NB][MAX_CARDINALITY];
	int pieces[COLOR_NB][MAX_CARDINALITY];
	size_t num_pieces[COLOR_NB] = { 0, 0 };
	for (int sq = SQUARE_ZERO; sq < SQUARE_NB; sq++)
	{
		auto piece = pos.piece_on((Square)sq);
		if (piece == NO_PIECE)
			continue;
		auto color = color_of(piece);
		if (num_pieces[color] >= MAX_CARDINALITY)
			return -1000;
		squares[color][num_pieces[color]] = sq;
		pieces[color][num_pieces[color]] = piece;
		num_pieces[color]++;
	}
	dtw = 0;
	return atomic_tb_probe_dtw(squares[WHITE], pieces[WHITE], num_pieces[WHITE],
	                           squares[BLACK], pieces[BLACK], num_pieces[BLACK],
	                           pos.side_to_move(), pos.ep_square(), &dtw);
}


int tests()
{
	using Test = std::tuple<std::string, int16_t, int>;
	// The longest line of each table with 3-4 pieces and some with 5 (generator statistics; atomic
	// games end with the winner's move, so an odd length is a win for the side to
	// move, an even length a loss), and special cases
	auto tests = {
		Test("8/8/8/8/8/8/8/kBBK4 w - - 0 1",          1,  0), // KBBvK
		Test("8/8/8/8/8/2N5/8/k1BK4 w - - 0 1",        1,  0), // KBNvK
		Test("8/8/8/8/8/K7/Pk6/B7 b - - 0 1",        -34,  0), // KBPvK
		Test("8/8/8/8/8/8/1B6/bkK5 w - - 0 1",         1,  0), // KBvKB
		Test("8/8/8/8/8/8/1n6/k3BK2 w - - 0 1",        3,  0), // KBvKN
		Test("8/5B2/8/8/k7/8/3p4/K7 b - - 0 1",       51,  0), // KBvKP
		Test("8/8/K7/8/8/k7/P7/7N b - - 0 1",        -36,  0), // KNPvK
		Test("8/8/8/8/8/8/2N5/nkK5 w - - 0 1",         1,  0), // KNvKN
		Test("8/8/1N6/8/5k2/8/1p6/7K b - - 0 1",      33,  0), // KNvKP
		Test("8/8/8/8/7K/Pk6/P7/8 b - - 0 1",        -36,  0), // KPPvK
		Test("2K5/8/8/8/1P6/8/7k/8 w - - 0 1",        23,  0), // KPvK
		Test("3K4/8/8/4p3/8/4k3/1P6/8 w - - 0 1",     61,  0), // KPvKP
		Test("8/8/8/8/8/3K3B/8/Qk6 b - - 0 1",       -20,  0), // KQBvK
		Test("8/8/7N/8/8/8/1K6/Qk6 b - - 0 1",       -22,  0), // KQNvK
		Test("8/8/8/8/5K2/P7/8/Q1k5 b - - 0 1",      -24,  0), // KQPvK
		Test("8/8/8/8/8/1K6/8/1k4QQ b - - 0 1",      -16,  0), // KQQvK
		Test("7Q/8/8/8/3K4/8/8/Rk6 b - - 0 1",       -18,  0), // KQRvK
		Test("8/7Q/8/8/8/4K3/8/1k6 b - - 0 1",       -22,  0), // KQvK
		Test("8/8/6b1/8/Q7/8/8/k2K4 b - - 0 1",      -50,  0), // KQvKB
		Test("8/8/8/Q5n1/8/8/3K4/2k5 w - - 0 1",      33,  0), // KQvKN
		Test("8/1K4Q1/k7/8/3p4/8/8/8 b - - 0 1",     -60,  0), // KQvKP
		Test("2Q5/3q4/8/8/5k2/8/8/1K6 w - - 0 1",     15,  0), // KQvKQ
		Test("8/8/8/8/8/2r5/8/Q1K2k2 w - - 0 1",      25,  0), // KQvKR
		Test("8/8/8/8/8/8/K7/Rk4B1 b - - 0 1",       -38,  0), // KRBvK
		Test("7R/7N/8/8/2K5/8/2k5/8 b - - 0 1",      -36,  0), // KRNvK
		Test("8/8/8/8/K7/8/Pk6/R7 b - - 0 1",        -28,  0), // KRPvK
		Test("8/8/8/8/4K3/8/8/RRk5 b - - 0 1",       -20,  0), // KRRvK
		Test("8/8/8/8/8/3R4/1kb5/4K3 w - - 0 1",       9,  0), // KRvKB
		Test("8/8/8/8/2R5/8/8/nk1K4 w - - 0 1",       11,  0), // KRvKN
		Test("8/8/1p6/8/8/6RK/6k1/8 b - - 0 1",     -124,  0), // KRvKP
		Test("8/8/8/8/8/8/3RK3/1kr5 w - - 0 1",        7,  0), // KRvKR
		// 5 pieces: the longest line (a blessed loss: return value 1) and the longest
		// within the 50-move rule of KQPvKN; KRBvKN: mated at ply 100, still in time
		Test("5Q2/8/8/8/8/3P4/n7/kK6 b - - 0 1",     -130,  1), // KQPvKN
		Test("8/5Q2/2P5/1n2k3/3K4/8/8/8 b - - 0 1",  -108,  0), // KQPvKN
		Test("8/4K3/8/4B3/1R6/8/1n6/2k5 b - - 0 1",  -100,  0), // KRBvKN
		// A blessed loss whose only escape is Kb2 next to the white king (the other moves
		// lose within the 50-move rule, with a longer DTW)
		Test("8/8/8/8/8/8/K6Q/1nk1N3 b - - 0 1",     -102,  1), // KQNvKN
		// The winner's slower line within the 50-move rule (1-ply bound from the
		// successors' counter-0 values: 104)
		Test("8/8/8/K7/1p6/8/8/4R1k1 b - - 0 1",    -118,  0),
		// The longest lines per category (3-5 pieces, pawns / pawnless, within the 50-move
		// rule / cursed, see line_tests()) not covered above: KRvKP cursed, KQNvKN within
		// the 50-move rule and cursed, KPPvKB, KNPvKN cursed
		Test("8/K7/8/8/1p6/8/8/6Rk b - - 0 1",      -122,  1), // KRvKP
		Test("7N/8/8/8/8/8/8/knKQ4 b - - 0 1",      -104,  0), // KQNvKN
		Test("6K1/7k/8/8/5Q2/8/2n1N3/8 b - - 0 1",  -126,  1), // KQNvKN
		Test("kb6/8/8/P7/3P4/3K4/8/8 w - - 0 1",     145,  0), // KPPvKB
		Test("8/6k1/7K/2P5/6n1/8/8/6N1 w - - 0 1",   167,  1), // KNPvKN
		// Checkmated: a loss in 0 plies (stored as 0 like draws in the compressed files,
		// recognized by the reader). With the white king on b6 Ka7 would escape:
		// adjacent kings can't give check, so k7/1Q6/1K6/8/8/8/8/8 b is a draw.
		Test("k7/1Q6/2K5/8/8/8/8/8 b - - 0 1",         0,  0),
		Test("k7/1Q6/1K6/8/8/8/8/8 b - - 0 1",         0,  2),
		// Draws: KRvK, bare kings
		Test("8/8/8/8/8/8/1R6/K6k w - - 0 1",          0,  2),
		Test("8/8/8/8/8/8/8/K6k w - - 0 1",            0,  2),
		// En passant capture next to the white king explodes it
		Test("8/8/8/8/Pp6/8/1K6/7k b - a3 0 1",        1,  0),
		// Illegal: the side not to move is in check
		Test("8/8/8/8/8/8/6Q1/K6k w - - 0 1",          0, -7),
	};
	bool is_error = false;
	for (auto& [fen, dtw_res, ec] : tests)
	{
		StateInfo st;
		Position pos;
		pos.set(fen, false, ATOMIC_VARIANT, &st, Threads.main());
		int dtw = 0;
		std::cout << "Test " << std::setw(40) << std::left << fen << ": ";
		int res = probe(pos, dtw);
		if (res != ec)
			std::cout << "Error code " << ec << " != " << res << " --> ERROR";
		else if (dtw != dtw_res)
			std::cout << "DTW " << dtw << " != " << dtw_res << " --> ERROR";
		else
			std::cout << "PASSED";
		std::cout << std::endl;
		if (res != ec || dtw != dtw_res)
			is_error = true;
	}
	return is_error ? -110 : 0;
}


// Legal move of pos written as san (e.g. "Nf3", "exd6", "Rxb2#", "a8=Q", "Qbc5"),
// MOVE_NONE if none or ambiguous
Move san_to_move(const Position& pos, std::string san)
{
	while (!san.empty() && (san.back() == '+' || san.back() == '#'))
		san.pop_back();
	PieceType promotion = NO_PIECE_TYPE;
	auto eq = san.find('=');
	if (eq != std::string::npos)
	{
		const std::string types = "  NBRQ"; // index = PieceType
		auto t = (eq + 1 < san.size()) ? types.find(san[eq + 1]) : std::string::npos;
		if (t == std::string::npos || t < KNIGHT)
			return MOVE_NONE;
		promotion = PieceType(t);
		san = san.substr(0, eq);
	}
	if (san.size() < 2)
		return MOVE_NONE;
	const std::string to_str = san.substr(san.size() - 2);
	if (to_str[0] < 'a' || to_str[0] > 'h' || to_str[1] < '1' || to_str[1] > '8')
		return MOVE_NONE;
	const Square to = make_square(File(to_str[0] - 'a'), Rank(to_str[1] - '1'));
	std::string rest = san.substr(0, san.size() - 2);
	PieceType pt = PAWN;
	const std::string letters = "  NBRQK"; // index = PieceType
	if (!rest.empty() && std::isupper((unsigned char)rest[0]))
	{
		auto t = letters.find(rest[0]);
		if (t == std::string::npos || t < KNIGHT)
			return MOVE_NONE;
		pt = PieceType(t);
		rest = rest.substr(1);
	}
	rest.erase(std::remove(rest.begin(), rest.end(), 'x'), rest.end());
	Move found = MOVE_NONE;
	for (const auto& m : MoveList<LEGAL>(pos))
	{
		const Square from = from_sq(m.move);
		if (to_sq(m.move) != to || type_of(pos.moved_piece(m.move)) != pt)
			continue;
		if ((type_of(m.move) == PROMOTION ? promotion_type(m.move) : NO_PIECE_TYPE) != promotion)
			continue;
		bool ok = true;
		for (char ch : rest)
		{
			if (ch >= 'a' && ch <= 'h')
				ok = ok && file_of(from) == File(ch - 'a');
			else if (ch >= '1' && ch <= '8')
				ok = ok && rank_of(from) == Rank(ch - '1');
			else
				ok = false;
		}
		if (!ok)
			continue;
		if (found != MOVE_NONE)
			return MOVE_NONE; // ambiguous
		found = m.move;
	}
	return found;
}


// The longest line of each category (3, 4, 5 pieces; pawnless / with pawns; within the
// 50-move rule / cursed), with the exact value at every 50-move counter.
// Every position of a line is probed:
// - the side to move wins (odd number of plies left) or loses (even), no draw;
// - the stored value (for counter 0) is at most the plies left (a win can take longer
//   at a higher counter, never shorter);
// - it equals the plies left at the start, after a capture or pawn move (counter 0),
//   and in cursed positions (return value 1: the cursed part of a line follows the
//   stored values);
// - the line ends with the game: checkmate (value 0) or an exploded king.
// "{TABLE}" in a line: the line continues in another table; "{counter 0}": a cursed
// line reaches a position decided within the 50-move rule, counted from counter 0 (as
// its stored value).
int line_tests()
{
	using Line = std::tuple<std::string, int, std::string>; // FEN, plies, moves
	const Line lines[] = {
		// 3 pieces, pawnless (KQvK)
		Line("8/7Q/8/8/8/4K3/8/1k6 b - - 0 1", 22,
			"Kc1 2.Kd4 Kd2 3.Ke5 Ke3 4.Kf6 Kf4 5.Kg7 Kg5 6.Kh8 Kf6 7.Qg7 Kf5 8.Qf6 Ke4 9.Qe5 Kd3 10.Qd4 Kc2 "
			"11.Qc3 Kb1 12.Qb2#"),
		// 3 pieces, pawns (KPvK)
		Line("2K5/8/8/8/1P6/8/7k/8 w - - 0 1", 23,
			"1.b5 Kg3 2.b6 Kf4 3.b7 Ke5 4.b8=Q {KQvK} Ke6 5.Kb7 Kd5 6.Ka8 Kc6 7.Qb7 Kd6 8.Qc6 Ke5 9.Qd5 Kf6 "
			"10.Qe5 Kf7 11.Qf6 Kg8 12.Qg7#"),
		// 4 pieces, pawnless (KQvKB)
		Line("8/8/6b1/8/Q7/8/8/k2K4 b - - 0 1", 50,
			"Kb1 2.Ke1 Kc1 3.Kf2 Kd2 4.Kg3 Ke3 5.Kh4 Kf3 6.Qg4 Kg3 7.Kg5 Kf4 8.Kh6 Kg5 9.Kg7 Kf6 10.Kh8 Ke7 "
			"11.Qf4 Bf5 12.Kg7 Kf7 13.Kh6 Kg7 14.Kg5 Kg6 15.Kh4 Kh5 16.Kg3 Kh4 17.Kf2 Kg3 18.Ke3 Kf2 19.Kd4 Ke3 "
			"20.Ke5 Ke4 21.Kf6 Ke5 22.Kg5 Kf6 23.Kh4 Kg7 24.Qg5 Kf7 25.Qf6 Kg8 26.Qg7#"),
		// 4 pieces, pawns, within the 50-move rule (KRvKP)
		Line("8/8/1p6/8/8/6RK/6k1/8 b - - 0 1", 124,
			"b5 2.Kh4 Kh3 3.Rb3 b4 4.Kg4 Kg3 5.Kf4 Kf3 6.Ke4 Ke3 7.Kd3 Kd2 8.Kc2 Kd3 9.Kb2 Ke4 10.Re3 Kd4 "
			"11.Rd3 Kc5 12.Rd5 Kb6 13.Kb3 Kc6 14.Ra5 Kd7 15.Ra6 Kc8 16.Ra7 Kb8 17.Rb7 Kc8 18.Rc7 Kd8 19.Ra7 Kc8 "
			"20.Rb7 Kd8 21.Rc7 Ke8 22.Rd7 Kf8 23.Re7 Kg8 24.Rf7 Kh8 25.Rf1 Kg8 26.Rf2 Kg7 27.Rf8 Kg6 28.Rf7 Kg5 "
			"29.Rf6 Kg4 30.Rf5 Kh3 31.Rf4 Kg3 32.Rd4 Kf3 33.Re4 Kg3 34.Rf4 Kh3 35.Rg4 Kh2 36.Rg3 Kh1 37.Rd3 Kh2 "
			"38.Re3 Kg2 39.Rh3 Kf2 40.Rg3 Ke2 41.Rf3 Kd1 42.Rf2 Ke1 43.Ra2 Kd1 44.Rd2 Ke1 45.Kb2 Kf1 46.Re2 Kg1 "
			"47.Rf2 Kh1 48.Rg2 b3 49.Rd2 Kg1 50.Rh2 Kf1 51.Rg2 Ke1 52.Rf2 Kd1 53.Re2 Kc2 54.Rd2 Kc3 55.Rd4 Kc2 "
			"56.Rd3 Kb1 57.Ka3 Ka1 58.Rc3 Kb1 59.Ka4 Ka1 60.Rc1 Ka2 61.Rb1 Ka3 62.Ka5 b2 63.Rxb2#"),
		// 4 pieces, pawns, cursed (KRvKP)
		Line("8/K7/8/8/1p6/8/8/6Rk b - - 0 1", 122,
			"Kh2 2.Rg2 Kh3 {counter 0} 3.Rg3 Kh4 4.Rb3 Kh5 5.Ka6 Kg4 6.Ka5 Kg5 7.Ka4 Kf4 8.Rf3 Ke5 9.Kb3 Kd4 "
			"10.Rf1 Kc4 11.Rf4 Kb5 12.Rf5 Kc6 13.Ra5 Kd7 14.Ra6 Kc8 15.Ra7 Kb8 16.Rb7 Kc8 17.Rc7 Kd8 18.Ra7 Kc8 "
			"19.Rb7 Kd8 20.Rc7 Ke8 21.Rd7 Kf8 22.Re7 Kg8 23.Rf7 Kh8 24.Rf1 Kg8 25.Rf2 Kg7 26.Rf8 Kg6 27.Rf7 Kg5 "
			"28.Rf6 Kg4 29.Rf5 Kh3 30.Rf4 Kg3 31.Rd4 Kf3 32.Re4 Kg3 33.Rf4 Kh3 34.Rg4 Kh2 35.Rg3 Kh1 36.Rd3 Kh2 "
			"37.Re3 Kg2 38.Rh3 Kf2 39.Rg3 Ke2 40.Rf3 Kd1 41.Rf2 Ke1 42.Ra2 Kd1 43.Rd2 Ke1 44.Kb2 Kf1 45.Re2 Kg1 "
			"46.Rf2 Kh1 47.Rg2 b3 48.Rd2 Kg1 49.Rh2 Kf1 50.Rg2 Ke1 51.Rf2 Kd1 52.Re2 Kc2 53.Rd2 Kc3 54.Rd4 Kc2 "
			"55.Rd3 Kb1 56.Ka3 Ka1 57.Rc3 Kb1 58.Ka4 Ka1 59.Rc1 Ka2 60.Rb1 Ka3 61.Ka5 b2 62.Rxb2#"),
		// 5 pieces, pawnless, within the 50-move rule (KQNvKN)
		Line("7N/8/8/8/8/8/8/knKQ4 b - - 0 1", 104,
			"Kb2 2.Qd3 Kc2 3.Kd2 Na3 4.Ke3 Kd2 5.Ng6 Nb5 6.Nf4 Ke2 7.Ke4 Ke3 8.Qd4 Kf3 9.Qe3 Nd6 10.Qd3 Ne8 "
			"11.Qd4 Nc7 12.Ke5 Ke4 13.Kd5 Ke5 14.Ne6 Ke4 15.Kc6 Kd5 16.Qb6 Ne8 17.Kd6 Ke5 18.Ke7 Kf6 19.Qb8 Nc7 "
			"20.Qb3 Nb5 21.Qd3 Nc7 22.Kd7 Ke7 23.Kd6 Na6 24.Qb5 Nc7 25.Qa4 Nd5 26.Qd4 Nb6 27.Nd8 Ke6 28.Kc5 Kd5 "
			"29.Kb5 Kc6 30.Ka6 Kb5 31.Kb7 Nc8 32.Qc5 Kb6 33.Kc6 Kc7 34.Kd6 Na7 35.Kd5 Kc6 36.Kc4 Kd5 37.Kb4 Kc4 "
			"38.Kb5 Kb4 39.Nb7 Ka5 40.Kb6 Kb5 41.Kc6 Kb6 42.Kd5 Kc6 43.Nd6 Nc8 44.Ke6 Kd5 45.Kd7 Ke6 46.Kc6 Na7 "
			"47.Kb6 Kd7 48.Ka5 Ke7 49.Nc8 Kf6 50.Nxa7 {KQvK} Kg7 51.Qf2 Kh8 52.Qg1 Kh7 53.Qg7#"),
		// 5 pieces, pawnless, cursed (KQNvKN)
		Line("6K1/7k/8/8/5Q2/8/2n1N3/8 b - - 0 1", 126,
			"Nd4 2.Kf7 Kg6 3.Ke7 Kf6 4.Kd6 Ke7 5.Kd5 Kd6 6.Kc4 Kc5 7.Kc3 Kb4 8.Kb2 Ka3 9.Kc1 Kb3 10.Qg3 Kc2 "
			"11.Nc3 Ne2 12.Qf3 Ng1 13.Qd3 Ne2 14.Kd2 {counter 0} Kd1 15.Nb5 Nc3 16.Ke3 Ke2 17.Kd4 Ke3 18.Nd6 Nb5 "
			"19.Ne4 Nc7 20.Nd2 Ne8 21.Kc3 Kd4 22.Kc2 Kc3 23.Kd1 Kc2 24.Ke2 Kd1 25.Qd5 Nf6 26.Qd6 Ng4 27.Qe6 Ke1 "
			"28.Kf2 Kf1 29.Kg3 Kg2 30.Nf3 Nf2 31.Kf4 Kg3 32.Qf5 Kg4 33.Ke5 Kf4 34.Ng5 Nd1 35.Kf6 Ke5 36.Kg6 Kf6 "
			"37.Kh5 Kg7 38.Qd3 Kg6 39.Kh4 Kh5 40.Kg4 Kh4 41.Kf3 Kg3 42.Ke2 Kf2 43.Ke3 Nb2 44.Qd4 Ke2 45.Ke4 Kf3 "
			"46.Kd5 Ke4 47.Kc5 Kd5 48.Kc4 Kc5 49.Ne4 Kb4 50.Nc3 Kb3 51.Qe3 Kb4 52.Qe4 Kb3 53.Qd4 Kb4 54.Kb3 Kc4 "
			"55.Qe4 Kb4 56.Kc2 Kb3 57.Qe2 Nd3 58.Kd1 Ne1 59.Qc2 Kc4 60.Qe4 Kb3 61.Ke2 Ng2 62.Qa4 Kb2 63.Qc2 Ka1 "
			"64.Qb1#"),
		// 5 pieces, pawns, within the 50-move rule (KPPvKB)
		Line("kb6/8/8/P7/3P4/3K4/8/8 w - - 0 1", 145,
			"1.a6 Ka7 2.Ke2 Kb6 3.a7 Kb5 4.a8=Q {KQPvKB} Kc4 5.Kf3 Kd3 6.Kg4 Ke3 7.Qc6 Ba7 8.Kf5 Kf4 9.Ke6 Kf5 "
			"10.Kd6 Ke6 11.Kc7 Kd7 12.Kb7 Kc8 13.Ka6 Kb7 14.Kb6 Bb8 15.Kc5 Kb6 16.Kd5 Kc5 17.Kd6 Kd5 18.Kd7 Kd6 "
			"19.Kc7 Ba7 20.Kb7 Kc7 21.Kb6 Bb8 22.d5 Kb7 23.Kc5 Kb6 24.Kd4 Kc5 25.Ke4 Kd4 26.Ke5 Ke4 27.Ke6 Ke5 "
			"28.Kd6 Ba7 29.Kc7 Kd6 30.Kb7 Kc7 31.Kb6 Bb8 32.Qa4 Kd6 33.Kb7 Ba7 34.Ka6 Kc5 35.d6 Kb6 36.Kb7 Kc6 "
			"37.Kb6 Kd5 38.Ka6 Kc5 39.Qa5 Kb6 40.Kb7 Kc6 41.Kb6 Bb8 42.Qa6 Ba7 43.d7 Kd6 44.Ka5 Kc5 45.Qb5 Kb6 "
			"46.d8=Q Bb8 47.Qc7 Ka6 48.Kb4 Ka5 49.Kc4 Kb4 50.Kd5 Kc5 51.Ke6 Kd5 52.Qd6 Ba7 53.Kd7 Kc6 54.Qb6 Kc7 "
			"55.Kc6 Kd7 56.Kd5 Kc6 57.Qbc5 Bb8 58.Ke5 Kd5 59.Ke6 Ba7 60.Qd4 Ke5 61.Kd5 Ke6 62.Kc6 Kd7 63.Kb5 Kc6 "
			"64.Qb6 Bb8 65.Ka6 Kb5 66.Kb7 Kc6 67.Qdc7 Ba7 68.Ka6 Kb5 69.Qcc6 Bb8 70.Kb7 Ka6 71.Kc8 Kb7 72.Qcc7 Ba7 "
			"73.Qxa7#"),
		// 5 pieces, pawns, cursed (KNPvKN)
		Line("8/6k1/7K/2P5/6n1/8/8/6N1 w - - 0 1", 167,
			"1.c6 Kh8 2.Kh7 Nh2 3.c7 Nf3 4.Ne2 Nd4 5.Nc1 Nb3 6.Na2 Nc1 7.Nb4 Nd3 8.Nc2 Nb4 9.Na1 Nc2 10.Nb3 Nd4 "
			"11.Nd2 Nf3 12.Nf1 Nd2 13.Nh2 Nf3 14.Ng4 Ne5 15.Nf2 Nd3 16.Nh3 Nf4 17.Ng1 Ne2 18.Nf3 Nd4 19.Ne1 Nc2 "
			"20.Nd3 Nb4 21.Nc1 Nd3 22.Ne2 Nf4 23.Ng3 Nh5 24.Nf1 Ng3 25.Nh2 Nf1 26.Nf3 Nd2 27.Nd4 Nb3 28.Ne6 Kg7 "
			"29.Kg6 Nd4 30.Nf8 Kf6 31.c8=Q {KQNvKN} Kf5 32.Ne6 Kg5 33.Nf4 Kf5 34.Qc4 Nc2 35.Kg5 Na3 36.Qf1 Nb1 "
			"37.Qd3 Na3 38.Kg4 Nb5 39.Kg3 Kg4 40.Kf3 Nc7 41.Ke3 Kf3 42.Ke4 Ne8 43.Qd4 Nc7 44.Ke5 Ke4 45.Kd5 Ke5 "
			"46.Ne6 Ke4 47.Kc6 Kd5 48.Qb6 Ne8 49.Kd6 Ke5 50.Qd4 Nc7 51.Qd5 Na6 52.Kc6 Kd6 53.Qc5 Kc7 54.Qc4 Nb8 "
			"55.Kc5 Kb6 56.Qd5 Kc6 57.Qd3 Na6 58.Qb5 Nb8 59.Qb7 Kb6 60.Kc6 Kc5 61.Kc7 Kc6 62.Kd8 Kd6 63.Qb6 Kd7 "
			"64.Qb5 Na6 65.Kc7 Kc6 66.Kb6 Nb8 67.Nc5 Kb7 68.Ka5 Ka6 69.Kb4 Ka5 70.Kc4 Kb4 71.Kd5 Kc4 72.Kd6 Kd5 "
			"73.Kc6 Kd6 74.Qb7 Kd5 75.Qa7 Nd7 76.Kb7 Nb6 77.Qa6 Kc6 78.Kb8 Kd6 79.Na4 Ke7 80.Qb7 Ke6 81.Qd5 Ke7 "
			"82.Qd6 Kf7 83.Qf6 Kg8 84.Qg7#"),
	};
	bool is_error = false;
	for (const auto& [fen, plies, moves] : lines)
	{
		std::cout << "Line " << std::setw(40) << std::left << fen << ": ";
		std::deque<StateInfo> states(1);
		Position pos;
		pos.set(fen, false, ATOMIC_VARIANT, &states.back(), Threads.main());
		std::istringstream tokens(moves);
		std::string tok, error;
		int left = plies;   // plies to the end of the game
		bool exact = true;  // start or counter 0: the stored value is the plies left
		int num_probed = 0;
		while (error.empty())
		{
			if (pos.count<KING>(WHITE) == 0 || pos.count<KING>(BLACK) == 0)
			{
				if (left != 0)
					error = "a king exploded with " + std::to_string(left) + " plies left";
				break;
			}
			int dtw = 0;
			const int res = probe(pos, dtw);
			num_probed++;
			std::stringstream ss;
			ss << pos.fen() << " (" << left << " plies left): ";
			if (res != 0 && res != 1)
				error = ss.str() + "result code " + std::to_string(res);
			else if (left == 0 ? dtw != 0 : (left % 2 == 1) != (dtw > 0))
				error = ss.str() + "wrong side wins: DTW " + std::to_string(dtw);
			else if (std::abs(dtw) > left)
				error = ss.str() + "DTW " + std::to_string(dtw) + " is longer than the line";
			else if ((exact || res == 1) && std::abs(dtw) != left)
				error = ss.str() + "DTW " + std::to_string(dtw) + " != plies left";
			if (!error.empty() || left == 0)
				break;
			// The next move ("12." numbers and "{...}" notes skipped)
			while (tokens >> tok && tok[0] == '{')
				while (tok.back() != '}' && tokens >> tok)
					;
			if (!tokens)
			{
				error = "the line ends with " + std::to_string(left) + " plies left";
				break;
			}
			auto dot = tok.find('.');
			if (dot != std::string::npos)
				tok = tok.substr(dot + 1);
			const Move m = san_to_move(pos, tok);
			if (m == MOVE_NONE)
			{
				error = "no legal move " + tok + " in " + pos.fen();
				break;
			}
			exact = pos.capture(m) || type_of(pos.moved_piece(m)) == PAWN;
			states.emplace_back();
			pos.do_move(m, states.back());
			left--;
		}
		if (error.empty() && (tokens >> tok))
			error = "moves after the end of the game: " + tok;
		if (error.empty())
			std::cout << "PASSED (" << plies << " plies, " << num_probed << " positions)" << std::endl;
		else
		{
			std::cout << error << " --> ERROR" << std::endl;
			is_error = true;
		}
	}
	return is_error ? -111 : 0;
}


int main(int argc, char* argv[])
{
	if (argc < 2)
		return -101;
	std::string egtb_path = argv[1];

	int res_init = atomic_tb_init();
	if (res_init != 0)
		return -102;
	int res_path = atomic_tb_add_path(egtb_path.c_str(), egtb_path.length());
	if (res_path != 0)
		return -103;

	// 8/7Q/8/8/8/4K3/8/1k6 b - - 0 1 (KQvK, black is mated in 22 plies)
	int white_squares[2] = { 20, 55 };
	int white_pieces[2] = { 6, 5 };
	size_t num_white_pieces = 2;
	int black_squares[1] = { 1 };
	int black_pieces[1] = { 6 };
	size_t num_black_pieces = 1;
	int stm = 1;
	int ep_square = 0;
	int dtw = 0;
	int res = atomic_tb_probe_dtw(white_squares, white_pieces, num_white_pieces,
	                              black_squares, black_pieces, num_black_pieces,
	                              stm, ep_square, &dtw);
	if (dtw != -22 || res != 0)
		return -104;

	// No white king
	int no_king_pieces[2] = { 5, 5 };
	res = atomic_tb_probe_dtw(white_squares, no_king_pieces, num_white_pieces,
	                          black_squares, black_pieces, num_black_pieces,
	                          stm, ep_square, &dtw);
	if (res != -6)
		return -105;

	res = tests();
	int res_lines = line_tests();
	int res_crc = crc_tests(egtb_path);
	return res != 0 ? res : res_lines != 0 ? res_lines : res_crc;
}
