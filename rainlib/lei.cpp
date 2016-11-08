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
  buf_top = (buf_top+1) % MAX_SIZE_BUFFER;
  buf[buf_top].src = src;
  buf[buf_top].tgt = tgt;
}

void LEI::formTrace(unsigned long long start, int old) {
  unsigned long long prev = start;

  rain::Region* r = nullptr;
  rain::Region::Node* last_node = NULL;
  for (int branch = old+1; (branch % MAX_SIZE_BUFFER) < buf_top; branch++) {
    unsigned long long branch_src = buf[branch % MAX_SIZE_BUFFER].src;
    unsigned long long branch_tgt = buf[branch % MAX_SIZE_BUFFER].tgt;

    auto it = instructions.find(prev);
    bool first = true;
    while (it != instructions.end() && *it != branch_src) {
        // Stop if branch forms a cycle
        if (rain.code_cache.count(*it) != 0) {
          if (r && last_node)
            rain.createInterRegionEdge(last_node, rain.code_cache[*it]);

          break;
        }

        if (!r) {
          r = rain.createRegion();
        }
        rain::Region::Node* node = new rain::Region::Node(*it);
        rain.insertNodeInRegion(node, r);

        if (!last_node) {
          rain.setEntry(node);
        } else {
          rain.setEntry(node);
          rain.setExit(node);
          r->createInnerRegionEdge(last_node, node);
        }
        last_node = node;
        if (*it == branch_src) {
          break;
        }
        ++it;
    }

    if (*it == branch_src) {
        if (!r) { 
          r = rain.createRegion();
        }
        rain::Region::Node* node = new rain::Region::Node(*it);
        rain.insertNodeInRegion(node, r);

        if (!last_node) {
          rain.setEntry(node);
        } else {
          rain.setEntry(node);
          rain.setExit(node);
          r->createInnerRegionEdge(last_node, node);
        }
        last_node = node;
        if (*it == branch_src) {
          break;
        }
    }
    // Stop if branch forms a cycle
    if (r) {
      if(r->getNode(branch_tgt) != nullptr || branch_tgt == prev) break;
    }

    prev = branch_tgt;
  }

  if (last_node)
    rain.setExit(last_node);
}

char unsigned last_len = 0;
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
  if (instructions.find(cur_addr) == instructions.end() && !is_system_instr(cur_addr))
    instructions.insert(cur_addr);

  // Profile instructions to detect hot code
  //
  bool profile_target_instr = false;
  if ((edg == rain.nte_loop_edge) && abs(cur_addr - last_addr) > last_len) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr && !is_system_instr(last_addr) && !is_system_instr(cur_addr)) {
    unsigned long long src = last_addr;
    unsigned long long tgt = cur_addr;

    circularBufferInsert(src, tgt);
    if (buf_hash.count(tgt) != 0) {
      int old = buf_hash[tgt];
      buf_hash[tgt] = buf_top;

      // if tgt ≤ src or old follows exit from code cache
      bool is_a_cache_exit = (rain.code_cache.count(buf[old].src) != 0 &&
                             rain.code_cache.count(buf[old].tgt) == 0);
      if (tgt <= src || is_a_cache_exit) {
        // increment counter c associated with tgt
        profiler.update(tgt);

        // if c = Tcyc
        if (profiler.is_hot(tgt)) {
          // Start region formation....
          RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);

          formTrace(tgt, old);

          // remove all elements of Buf after old
          for (int branch = old+1; (branch % MAX_SIZE_BUFFER) < buf_top; branch++)
            buf_hash.erase(buf[branch % MAX_SIZE_BUFFER].tgt);
          buf_top = (old+1) % MAX_SIZE_BUFFER;

          // recycle counter associated with tgt
          profiler.reset(tgt);
        }
      }
    } else {
      buf_hash[tgt] = buf_top;
    }
  }

  last_addr = cur_addr;
  last_len = cur_length;
}

void LEI::finish() {
}
