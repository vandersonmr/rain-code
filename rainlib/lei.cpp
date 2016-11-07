/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   - Edson Borin (edson@ic.unicamp.br), and                              *
 *   - Raphael Zinsly (raphael.zinsly@gmail.com)                           *
 *   - Vanderson Rosario (vandersonmr2@gmail.com)                          * 
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "rf_techniques.h"
#include <iostream>
#include "arglib.h"
#include <algorithm>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

void LEI::circularBufferInsert(unsigned long long src, unsigned long long tgt) {
  if (buf_top >= MAX_SIZE_BUFFER-2) 
    buf_top = 0;

  buf[++buf_top] = src;
  buf[++buf_top] = tgt;
}

void LEI::formTrace(unsigned long long start, int old) {
  recording_buffer.reset();
  unsigned long long prev = start;

  for (int branch = old+1; (branch % MAX_SIZE_BUFFER) < buf_top; branch += 2) {
    unsigned long long branch_src = buf[branch];

    auto it = std::lower_bound(instructions.begin(), instructions.end(), prev);
    while (it != instructions.end() && *it != buf[branch]) {
        if (code_cache.count(*it) != 0) break;
        recording_buffer.append(*it);
        ++it;
    }

    if (recording_buffer.contains_address(buf[branch])) break;

    prev = buf[branch+1];
  }
}

long long int total = 0;
void LEI::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg) {
    edg = rain.addNext(cur_addr);
  }
  rain.executeEdge(edg);

  RF_DBG_MSG("0x" << setbase(16) << cur_addr << endl);

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg == rain.nte_loop_edge) && abs(cur_addr - last_addr) > 7) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    unsigned long long src = last_addr;
    unsigned long long tgt = cur_addr;

    circularBufferInsert(src, tgt);
    if (buf_hash.count(tgt) != 0) {
      int old = buf_hash[tgt];
      buf_hash[tgt] = buf_top;
      if (tgt <= src) {
        profiler.update(tgt);
        if (profiler.is_hot(cur_addr)) {
          // Start region formation....
          RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);
          formTrace(tgt, old);
          for (auto I : recording_buffer.addresses)
            code_cache[I] = true;
          buildRegion();
          buf_top = old;
          profiler.reset(tgt);
        }
      }
    } else {
      buf_hash[tgt] = buf_top;
    }
  }

  last_addr = cur_addr;
}
