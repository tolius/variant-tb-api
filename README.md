# Antichess and atomic tablebase API

This is an API for reading data from the [antichess](https://en.wikipedia.org/wiki/Antichess) and [atomic chess](https://en.wikipedia.org/wiki/Atomic_chess) endgame tablebases (Lichess rules) with DTW (Depth To Win) metric.

One source tree builds either library; the variant is chosen at compile time:
- `ANTI`: the library `antichess_tb_api` (header `src/antichess_tb_api.h`, tables `.an1`, functions `antichess_tb_*`, C++ namespace `AntichessTb`);
- `ATOMIC`: the library `atomic_tb_api` (header `src/atomic_tb_api.h`, tables `.at1`, functions `atomic_tb_*`, C++ namespace `AtomicTb`).

The internal code is written in the namespace `VariantTb`, a macro (`src/tb/variant_tb.h`) that becomes `AntichessTb` or `AtomicTb`, so both libraries can be linked into one program.

The `.an1` / `.at1` tables store the DTW of every position and, only in the tables that need it, a reduced DTZ (whether a win or loss is decided within the 50-move rule). They give the same results as the `.an0` / `.at0` tables with the full DTZ and are about 30% smaller.

## Building

With CMake, set `EGTB_VARIANT` to `ANTI` (default) or `ATOMIC`, or use a preset (`x64-release-antichess`, `x64-release-atomic`, `linux-release-antichess`, `linux-release-atomic`, and the debug versions):
```
cmake --preset linux-release-atomic
cmake --build --preset linux-release-atomic
```

`-DWITH_TESTS=OFF` builds the library only.

The library needs zlib: the CMake build compiles zlib-ng (`src/tb/egtb/dictzip/zlib-ng`, target `zlibstatic`, zlib API) and links it to the programs that use the library. Other build systems compile `src/variant_tb_api.cpp`, `src/tb/*.cpp`, `src/tb/syzygy/tbprobe.cpp`, `src/tb/egtb/*.cpp` with `-DANTI` or `-DATOMIC` (and optionally `-DUSE_POPCNT`, `-DUSE_PEXT`), and `src/tb/egtb/dictzip/*.c` (`-DHAVE_MMAP`, on Unix also `-DHAVE_UNISTD_H`) with any zlib.

## Antichess

The API represents 3 functions:
1. **antichess_tb_init** should be called once at the very beginning. If initialisation is successful, the function returns 0.
2. **antichess_tb_add_path** should be called once before probing, specifying the path to the folder where all tablebase files (`.an1`) are located. If all 714 tables with 2-4 pieces are successfully loaded, the function returns 0; otherwise the number of missing tables as a negative value.
3. **antichess_tb_probe_dtw** is used for probing antichess positions:
- ***white_squares***/***black_squares*** should have values from 0 (a1), 1 (b1), etc. to 63 (h8).
- ***white_pieces***/***black_pieces*** should have values from 1 to 6 for PNBRQK (only the least significant three bits are taken into account).
- ***num_white_pieces***/***num_black_pieces*** should be 1+1, 1+2, 1+3, 2+1, 2+2, or 3+1.
- ***ep_square*** specifies the *en passant* target square (16...23 or 40...47) or any other value outside these ranges if there are none.

In case of an error, the function returns a negative value indicating the error code. If one side wins, the function returns 0. In case of a cursed win or a blessed loss (because of the 50-move rule), the function returns 1. In case of a draw, the function returns 2.
If there are no errors, the function sets the value of ***dtw***. The absolute value of ***dtw*** is equal to the number of plies before the win. The value is positive if the side to move wins, or negative if the side to move loses. ***dtw*** = 0 if it is a draw or the side to move has no legal moves.

## Atomic

The API represents 3 functions:
1. **atomic_tb_init** should be called once at the very beginning. If initialisation is successful, the function returns 0.
2. **atomic_tb_add_path** should be called once before probing, specifying the path to the folder where all tablebase files (`.at1`) are located. If all 145 tables with 3-5 pieces are successfully loaded, the function returns 0; otherwise the number of missing tables as a negative value.
3. **atomic_tb_probe_dtw** is used for probing atomic positions:
- ***white_squares***/***black_squares*** should have values from 0 (a1), 1 (b1), etc. to 63 (h8).
- ***white_pieces***/***black_pieces*** should have values from 1 to 6 for PNBRQK (only the least significant three bits are taken into account). Each side must have exactly one king.
- ***num_white_pieces***/***num_black_pieces*** should be 1+1, 1+2, 1+3, 1+4, 2+1, 2+2, 2+3, 3+1, 3+2, or 4+1 (the kings included). Two bare kings are a draw (there is no KvK table).
- ***ep_square*** specifies the *en passant* target square (16...23 or 40...47) or any other value outside these ranges if there are none.

In case of an error, the function returns a negative value indicating the error code (-6: not exactly one king per side; -7: illegal position, the side not to move is in check, which is allowed when the kings are adjacent). If one side wins, the function returns 0. In case of a cursed win or a blessed loss (because of the 50-move rule), the function returns 1. In case of a draw (including stalemate), the function returns 2.
If there are no errors, the function sets the value of ***dtw***. The absolute value of ***dtw*** is equal to the number of plies before the win, assuming the position was reached by a capture or a pawn move (50-move counter 0). The value is positive if the side to move wins, or negative if the side to move loses. ***dtw*** = 0 if it is a draw or the side to move is checkmated (return value 0).

Positions decided within the 50-move rule are solved exactly per 50-move counter by the generator: the stored DTW is the shortest win / longest defence at counter 0 that obeys the 50-move rule.

## License

This code is released under the GPLv3+ license.

The following code is used in the program:
- [Syzygy](https://github.com/mcostalba/Stockfish/tree/master/src/syzygy) (modified)
- [Multi-Variant Stockfish](https://github.com/ddugovic/Stockfish/tree/146269195b1b6a5e9d1121d9fd5767668a48a2a6) (modified)
- Dictzip from [dictd 1.12.1](https://sourceforge.net/projects/dict/) with [Tvangeste's tweaks](https://github.com/Tvangeste/dictzip-win32/tree/bb996c999e9f437b1abb98d941a0a7a98ba82f67) (modified)
- [zlib-ng 2.3.3](https://github.com/zlib-ng/zlib-ng/tree/2.3.3) (without test, doc, tools, configure, Makefile.in)
