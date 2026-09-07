#!/usr/bin/env python3
"""Generate bounded macro arities for the shared PPC bitfield declaration boundary."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
out=['''#pragma once

// List each complete storage unit in the original PowerPC MSB-first order:
//   AURORA_PPC_BITFIELD_GROUP(unsigned, (active, 1), (mode, 2), (, 29))
// The host declarations preserve original numeric masks, including multi-bit
// field values. Named fields and raw word aliases therefore share one layout.
// This describes native CPU words, not the byte order of serialized resources.
// Each group accepts 1-32 (name, width) pairs and must fill its storage type.
// An empty name declares padding. Adjacent storage units use separate groups.

#define AURORA_DETAIL_PPC_JOIN_I(a, b) a##b
#define AURORA_DETAIL_PPC_JOIN(a, b) AURORA_DETAIL_PPC_JOIN_I(a, b)
#define AURORA_DETAIL_PPC_UNPAREN(...) __VA_ARGS__
#define AURORA_DETAIL_PPC_FIELD_I(type, name, width) type name : width;
#define AURORA_DETAIL_PPC_FIELD_EXPAND(...) AURORA_DETAIL_PPC_FIELD_I(__VA_ARGS__)
#define AURORA_DETAIL_PPC_FIELD(type, field) \\
  AURORA_DETAIL_PPC_FIELD_EXPAND(type, AURORA_DETAIL_PPC_UNPAREN field)
#define AURORA_DETAIL_PPC_WIDTH_I(name, width) width
#define AURORA_DETAIL_PPC_WIDTH(field) AURORA_DETAIL_PPC_WIDTH_I field
''']
args=', '.join('_'+str(i) for i in range(1,33));nums=', '.join(str(i) for i in range(32,0,-1))
out += [f'#define AURORA_DETAIL_PPC_COUNT_I({args}, count, ...) count\n',f'#define AURORA_DETAIL_PPC_COUNT(...) AURORA_DETAIL_PPC_COUNT_I(__VA_ARGS__, {nums})\n\n']
for i in range(1,33):
 rest=', ...' if i>1 else ''
 forward='AURORA_DETAIL_PPC_FIELD(type, field)'+(f' AURORA_DETAIL_PPC_FORWARD_{i-1}(type, __VA_ARGS__)' if i>1 else '')
 reverse=(f'AURORA_DETAIL_PPC_REVERSE_{i-1}(type, __VA_ARGS__) ' if i>1 else '')+'AURORA_DETAIL_PPC_FIELD(type, field)'
 valid='(AURORA_DETAIL_PPC_WIDTH(field) > 0 && AURORA_DETAIL_PPC_WIDTH(field) <= sizeof(type) * 8)'+(f' && AURORA_DETAIL_PPC_VALID_{i-1}(type, __VA_ARGS__)' if i>1 else '')
 width='AURORA_DETAIL_PPC_WIDTH(field)'+(f' + AURORA_DETAIL_PPC_SUM_{i-1}(__VA_ARGS__)' if i>1 else '')
 out += [f'#define AURORA_DETAIL_PPC_FORWARD_{i}(type, field{rest}) {forward}\n',f'#define AURORA_DETAIL_PPC_REVERSE_{i}(type, field{rest}) {reverse}\n',f'#define AURORA_DETAIL_PPC_SUM_{i}(field{rest}) {width}\n',f'#define AURORA_DETAIL_PPC_VALID_{i}(type, field{rest}) {valid}\n']
out.append('''
#if defined(__MWERKS__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define AURORA_DETAIL_PPC_ORDER AURORA_DETAIL_PPC_FORWARD_
#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || defined(_WIN32)
#define AURORA_DETAIL_PPC_ORDER AURORA_DETAIL_PPC_REVERSE_
#else
#error "PPC bitfield declarations require a supported host bit allocation order"
#endif

// A typedef checks the full storage width without adding data members, and is
// also accepted by the original C++98 Metrowerks compiler used for Wii proofs.
#define AURORA_PPC_BITFIELD_GROUP(type, ...) \\
  typedef char AURORA_DETAIL_PPC_JOIN(aurora_bitfield_width_check_, __LINE__)[ \\
      (AURORA_DETAIL_PPC_JOIN(AURORA_DETAIL_PPC_SUM_, AURORA_DETAIL_PPC_COUNT(__VA_ARGS__))(__VA_ARGS__) == \\
       sizeof(type) * 8 && AURORA_DETAIL_PPC_JOIN(AURORA_DETAIL_PPC_VALID_, \\
      AURORA_DETAIL_PPC_COUNT(__VA_ARGS__))(type, __VA_ARGS__)) ? 1 : -1]; \\
  AURORA_DETAIL_PPC_JOIN(AURORA_DETAIL_PPC_ORDER, AURORA_DETAIL_PPC_COUNT(__VA_ARGS__))(type, __VA_ARGS__)
''')
(ROOT/'aurora/include/aurora/ppc_bitfield.hpp').write_text(''.join(out))
