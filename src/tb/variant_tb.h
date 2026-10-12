#ifndef VARIANT_TB_H_INCLUDED
#define VARIANT_TB_H_INCLUDED

// Variant of the build: ANTI (antichess, .an0 files) or ATOMIC (.at0 files).
// The code is written in the namespace VariantTb, which is AntichessTb or AtomicTb
// depending on the variant, so that both libraries can be linked into one program.
#if defined(ANTI) == defined(ATOMIC)
#error "define exactly one of ANTI and ATOMIC"
#endif
#ifdef ATOMIC
#define VariantTb AtomicTb
#else
#define VariantTb AntichessTb
#endif

#endif // #ifndef VARIANT_TB_H_INCLUDED
