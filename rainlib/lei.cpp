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

void LEI::circularBufferInsert(unsigned long long src, unsigned long long tgt, Region::Edge* e) {
  buf.push_back({src, tgt, e});
}

rain::Region::Node* LEI::
insertNode(rain::Region* r, rain::Region::Node* last_node, unsigned long long new_addr) {

  rain::Region::Node* node = new rain::Region::Node(new_addr);
  rain.insertNodeInRegion(node, r);

  //rain.setEntry(node);
  if (!last_node) {
    rain.setEntry(node);
  } else {
    r->createInnerRegionEdge(last_node, node);
  }

  return node;
}

void LEI::formTrace(unsigned long long start, int old) {
  unsigned long long prev = start;

  rain::Region* r = nullptr;
  rain::Region::Node* last_node = NULL;
  unsigned branch = old+1;
  while (branch < buf.size()) {
    unsigned long long branch_src = buf[branch].src;
    unsigned long long branch_tgt = buf[branch].tgt;

    auto it = instructions.find(prev);
    while (it != instructions.getEnd()) {

      if (is_region_addr_space(it->first)) {
        if (rain.region_entry_nodes.count(it->first) != 0)
          return;

        if (!r)
          r = rain.createRegion();

        last_node = insertNode(r, last_node, it->first);
        recording_buffer.append(it->first);
      }

      if (it->first == branch_src)
        break;

      ++it;
    }

    // Stop if branch forms a cycle
    if (r)
      if (r->getNode(branch_tgt) != nullptr) {
          //rain.setEntry(r->getNode(branch_tgt));
          break;
      }

    prev = branch_tgt;
    branch += 1;
  }

  if (last_node)
    rain.setExit(last_node);
}

bool LEI::is_followed_by_exit(int old) {
  if (old  >= 0) {
    if (buf[old].edge->src->region != NULL) {
      return true;
    }
  }
  return false;
}

unsigned t = 0;
unsigned t2 = 0;
char unsigned last_len = 0;
void LEI::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg) {
    edg = rain.addNext(cur_addr);
  }
  rain.executeEdge(edg);

  RF_DBG_MSG("0x" << setbase(16) << cur_addr << endl);

  // Add cur_addr to instructions set if it's not already there
  if (!instructions.hasInstruction(cur_addr))
    instructions.addInstruction(cur_addr, cur_opcode);

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg->tgt == rain.nte) && abs(cur_addr - last_addr) > last_len) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    unsigned long long src = last_addr;
    unsigned long long tgt = cur_addr;

    circularBufferInsert(src, tgt, edg);

    if (buf_hash.count(tgt) != 0) {
      int old = buf_hash[tgt];
      buf_hash[tgt] = buf.size()-1;

      // if tgt ≤ src or old follows exit from code cache
      bool is_a_cache_exit = is_followed_by_exit(old);
      if (tgt <= src || is_a_cache_exit) {
        // increment counter c associated with tgt
        profiler.update(tgt);

        // if c = Tcyc
        if (profiler.is_hot(tgt)) {
          // Start region formation....
          RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);

          formTrace(tgt, old);
          recording_buffer.reset();

          // remove all elements of Buf after old
          for (int branch = old+1; branch < buf.size(); branch++) {
            buf_hash.erase(buf[branch].tgt);
            buf.pop_back();
          }

          // recycle counter associated with tgt
          profiler.reset(tgt);
        }
      }
    } else {
      buf_hash[tgt] = buf.size()-1;
    }
  }

  last_addr = cur_addr;
  last_len = cur_length;
}
