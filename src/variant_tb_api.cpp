#ifdef ATOMIC
#include "atomic_tb_api.h"
#else
#include "antichess_tb_api.h"
#endif

#include "tb/types.h"
#include "tb/egtb/tb_reader.h"
#include "tb/egtb/elements.h"
#include "tb/endgame.h"
#include "tb/thread.h"
#include "tb/uci.h"
#include "tb/syzygy/tbprobe.h"
#include "tb/bitboard.h"
#include "tb/position.h"
#include "tb/search.h"

#include <string>


using namespace VariantTb;


namespace VariantTb::PSQT {
	void init();
}


namespace {

#ifdef ATOMIC
// All atomic tables with 3-5 pieces (there is no KvK table: bare kings are a draw)
constexpr size_t NUM_TABLES = 145;
constexpr size_t MAX_PIECES = 5;
#else
// All antichess tables with 2-4 pieces
constexpr size_t NUM_TABLES = 714;
constexpr size_t MAX_PIECES = 4;
#endif


int tb_init()
{
	try
	{
		//CommandLine::init(argc, argv);
		UCI::init(Options);
		Tune::init();
		PSQT::init();
		Bitboards::init();
		Position::init();
		Bitbases::init();
		Endgames::init();
		Threads.set(size_t(Options["Threads"]));
		Search::clear(); // After threads are up
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}

int tb_add_path(const char* path, size_t path_len)
{
	try
	{
		std::string egtb_path(path, path_len);
		Tablebases::init(TB_VARIANT, egtb_path);
		TB_Reader::init(egtb_path, true);
		size_t num_files = TB_Reader::tb_cache.size();
		if (num_files >= NUM_TABLES)
			return 0;
		return static_cast<int>(num_files) - static_cast<int>(NUM_TABLES);
	}
	catch (...)
	{
		return -999;
	}
}

int tb_probe_dtw(const int* white_squares, const int* white_pieces, size_t num_white_pieces,
                 const int* black_squares, const int* black_pieces, size_t num_black_pieces,
                 int side_to_move, int ep_square, int* dtw)
{
	try
	{
		StateInfo st;
		Position pos;
		pos.reset();
		st.reset();
		pos.replace_state(&st);
		if (num_white_pieces < 1 || num_black_pieces < 1)
			return -1;
		size_t num_pieces = num_white_pieces + num_black_pieces;
		if (num_pieces < 2 || num_pieces > MAX_PIECES)
			return -2;
		auto add_pieces = [&pos](Color color, const int* squares, const int* pieces, size_t num_pcs)
		{
			for (size_t i = 0; i < num_pcs; i++)
			{
				if (squares[i] < SQUARE_ZERO || squares[i] >= SQUARE_NB)
					return -3;
				int pt = type_of((Piece)pieces[i]);
				if (pt < PAWN || pt > KING)
					return -4;
				Piece piece = make_piece(color, (PieceType)pt);
				pos.put_piece(piece, (Square)squares[i]);
			}
			return 0;
		};
		int ret_val = add_pieces(WHITE, white_squares, white_pieces, num_white_pieces);
		if (ret_val != 0)
			return ret_val;
		ret_val = add_pieces(BLACK, black_squares, black_pieces, num_black_pieces);
		if (ret_val != 0)
			return ret_val;
		if ((ep_square >= SQ_A3 && ep_square <= SQ_H3) || (ep_square >= SQ_A6 && ep_square <= SQ_H6))
			st.epSquare = (Square)ep_square;
#ifdef ATOMIC
		// Atomic: one king per side (checked before set_state(), which needs the kings),
		// and the side not to move must not be in check (kings next to each other can't
		// give check)
		if (pos.count<KING>(WHITE) != 1 || pos.count<KING>(BLACK) != 1)
			return -6;
#endif
		pos.set_side_to_move(side_to_move ? BLACK : WHITE);
		pos.set_state(&st);
#ifdef ATOMIC
		const Color them = ~pos.side_to_move();
		if (!pos.kings_adjacent() && (pos.attackers_to(pos.square<KING>(them)) & pos.pieces(~them)))
			return -7;
#endif

		int16_t tb_val;
		uint8_t tb_dtz;
		bool is_error = TB_Reader::probe_EGTB(pos, tb_val, tb_dtz);
		if (is_error)
			return -5;

		if (abs(tb_val) >= VariantTb::DRAW)
		{
			*dtw = 0;
			return 2;
		}
		// Atomic: tb_val 0 means the side to move is checkmated (a loss in 0 plies)
		*dtw = tb_val;
		return (tb_dtz > 100) ? 1 : 0;
	}
	catch (const std::exception& e)
	{
		return -10;
	}
	catch (...)
	{
		return -11;
	}
}

} // namespace


#ifdef ATOMIC

int atomic_tb_init()
{
	return tb_init();
}

int atomic_tb_add_path(const char* path, size_t path_len)
{
	return tb_add_path(path, path_len);
}

int atomic_tb_probe_dtw(const int* white_squares, const int* white_pieces, size_t num_white_pieces,
                        const int* black_squares, const int* black_pieces, size_t num_black_pieces,
                        int side_to_move, int ep_square, int* dtw)
{
	return tb_probe_dtw(white_squares, white_pieces, num_white_pieces,
	                    black_squares, black_pieces, num_black_pieces,
	                    side_to_move, ep_square, dtw);
}

#else

int antichess_tb_init()
{
	return tb_init();
}

int antichess_tb_add_path(const char* path, size_t path_len)
{
	return tb_add_path(path, path_len);
}

int antichess_tb_probe_dtw(const int* white_squares, const int* white_pieces, size_t num_white_pieces,
                           const int* black_squares, const int* black_pieces, size_t num_black_pieces,
                           int side_to_move, int ep_square, int* dtw)
{
	return tb_probe_dtw(white_squares, white_pieces, num_white_pieces,
	                    black_squares, black_pieces, num_black_pieces,
	                    side_to_move, ep_square, dtw);
}

#endif
