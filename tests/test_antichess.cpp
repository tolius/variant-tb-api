#include "antichess_tb_api.h"

#include "tb/types.h"
#include "tb/thread.h"
#include "tb/egtb/tb_reader.h"
#include "crc_check.h"

#include <algorithm>
#include <tuple>
#include <cctype>
#include <locale>
#include <iostream>
#include <iomanip>
#include <charconv>


using namespace VariantTb;


int tests()
{
	constexpr size_t MAX_CARDINALITY = 4;
	using Test = std::tuple<std::string, int16_t, int>;
	auto tests = {
		Test("7k/8/5K2/8/8/1R6/P7/8 w - - 0 1",    50,  0),
		Test("8/8/8/8/8/3k4/8/1R6 b - - 0 1",     -35,  0),
		Test("8/8/8/8/8/P7/8/7k b - - 0 1",       -45,  0),
		Test("8/8/K7/8/8/6b1/8/3b4 w - - 0 1",    -47,  0),
		Test("8/4p3/8/8/8/P7/P7/8 w - - 99 105",   68,  0),
		Test("2Q2R2/8/8/8/8/8/8/1K5k w - - 0 1",  110,  0),
		Test("8/1P6/8/8/8/K2P4/8/2k5 w - - 0 1",  142,  0),
		Test("8/5N2/5B2/8/k7/8/8/2K5 w - - 0 1",  148,  1),
		Test("8/8/8/8/8/5P2/1B6/2N3k1 w - - 0 1", 174,  1),
		Test("8/8/8/8/8/5Pp1/3B4/6N1 w - - 0 1",  174,  1),
		Test("8/8/8/5N2/8/B4P2/8/6k1 w - - 0 1",  162,  1),
		Test("8/8/8/8/3N4/B4P2/8/6k1 b - - 0 1", -161,  1),
		Test("8/5P2/8/8/3N4/B7/8/7k w - - 0 1",    72,  0),
		Test("8/2NK4/8/8/8/B2k4/8/8 b - - 0 1",  -105,  1),
		Test("8/2NK4/8/8/8/B7/2k5/8 w - - 0 1",   104,  0),
		Test("8/8/8/8/k7/8/8/K3K1N1 w - - 0 1",   104,  0),
		Test("8/8/8/8/k7/8/4N3/K3K3 b - - 1 1",     0,  2),
		Test("8/8/8/8/1k6/8/4N3/K3K3 w - - 0 1",    0,  2),
		Test("8/p7/8/1P6/7p/7R/8/8 b - - 0 25",     4,  0),
		Test("8/8/8/pP6/7p/7R/8/8 w - a6 0 26",    -3,  0),
		Test("8/8/8/pP6/8/5p2/1P6/8 w - a6 0 26", -11,  0),
		Test("8/8/8/pP6/8/5p2/1P6/8 w - - 0 26",    0,  2),
		Test("3N4/8/8/6Br/2pP4/8/8/1Q6 b - - 0 1",  0, -2),
		Test("b7/8/7p/3b4/6B1/8/8/8 b - - 0 1",  6, 0),
		Test("1B6/4P3/8/8/8/2B5/2p5/8 w - - 0 1",  10, 0),
		Test("N7/5B2/6P1/8/8/6n1/8/8 b - - 0 1",  -21, 0),
		Test("n7/8/8/8/8/8/2P5/7K b - - 0 1",  -29, 0),
		Test("8/8/2p5/8/1P6/4r3/8/8 b - - 0 1",  6, 0)
	};
	bool is_error = false;
	for (auto& [fen, dtw_res, ec] : tests)
	{
		StateInfo st;
		Position pos;
		pos.set(fen, false, ANTI_VARIANT, &st, Threads.main());
		int squares[COLOR_NB][MAX_CARDINALITY];
		int pieces[COLOR_NB][MAX_CARDINALITY];
		size_t num_white_pieces = 0;
		size_t num_black_pieces = 0;
		int dtw = 0;
		for (int sq = SQUARE_ZERO; sq < SQUARE_NB; sq++)
		{
			auto piece = pos.piece_on((Square)sq);
			if (piece == NO_PIECE)
				continue;
			auto color = color_of(piece);
			size_t& num_pieces = (color == WHITE) ? num_white_pieces : num_black_pieces;
			squares[color][num_pieces] = sq;
			pieces[color][num_pieces] = piece;
			num_pieces++;
		}
		std::cout << "Test " << std::setw(40) << std::left << fen << ": ";
		int res = antichess_tb_probe_dtw(squares[WHITE], pieces[WHITE], num_white_pieces,
		                                 squares[BLACK], pieces[BLACK], num_black_pieces,
		                                 pos.side_to_move(), pos.ep_square(), &dtw);
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


int main(int argc, char* argv[])
{
	if (argc < 2)
		return -101;
	std::string egtb_path = argv[1];

	int res_init = antichess_tb_init();
	if (res_init != 0)
		return -102;
	int res_path = antichess_tb_add_path(egtb_path.c_str(), egtb_path.length());
	if (res_path != 0)
		return -103;

	// 3k4/8/5K2/8/8/1R6/P7/8 w - - 0 1
	int white_squares[3] = { 8, 17, 45 };
	int white_pieces[3] = { 1, 4, 6 };
	size_t num_white_pieces = 3;
	int black_squares[1] = { 59 };
	int black_pieces[1] = { 6 };
	size_t num_black_pieces = 1;
	int stm = 0;
	int ep_square = 0;
	int dtw = 0;
	int res = antichess_tb_probe_dtw(white_squares, white_pieces, num_white_pieces,
	                                 black_squares, black_pieces, num_black_pieces,
	                                 stm, ep_square, &dtw);
	if (dtw != 50 || res != 0)
		return -104;

	res = tests();
	int res_crc = crc_tests(egtb_path);
	return res != 0 ? res : res_crc;
}
