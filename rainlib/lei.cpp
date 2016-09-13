/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   - Edson Borin (edson@ic.unicamp.br), and                              *
 *   - Raphael Zinsly (raphael.zinsly@gmail.com)                           *
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

bool LEI::hasRecorded(unsigned long long addr) {
  if (recorded.count(addr) == 0) 
    return false;
  return true;
}

int indx = 0;
void LEI::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg) {
    edg = rain.addNext(cur_addr);
  }
  rain.executeEdge(edg);

  if (history_buffer.addresses.size() > 1000000)
    history_buffer.addresses.erase(
        history_buffer.addresses.begin(), history_buffer.addresses.begin()+100000);

  history_buffer.append(cur_addr);

  RF_DBG_MSG("0x" << setbase(16) << cur_addr << endl);

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg->src->region != NULL) && edg->tgt == rain.nte) {
    // Profile NTE instructions that are target of regions instructions
    // Region exits
    profile_target_instr = true;
  }
  else if ((edg == rain.nte_loop_edge) && (cur_addr <= last_addr)) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording) {
      // Start region formation....
      RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);
      recording = true;
      recording_buffer.reset();
    }
  }

  if (recording) {
    bool paused = false;

    unsigned long long first_addr = -1;
    auto rit = history_buffer.addresses.rbegin();
    for (; rit != history_buffer.addresses.rend(); ++rit) { 
      unsigned long long int addr = (*rit);
      if (first_addr == -1) {
        first_addr = addr;
        continue;
      }

      if (hasRecorded(addr)) {
        recording_buffer.reset();
        break;
      }

      if (recording_buffer.addresses.size() > 1) {
        if (switched_mode(recording_buffer.addresses.back(), addr)) {
            paused = !paused;
        }
      }

      //if (!paused) 
        recording_buffer.append(addr);

      if (addr == first_addr)
        break;
    }

    if (recording_buffer.addresses.size() != 0) {
      std::reverse(recording_buffer.addresses.begin(), recording_buffer.addresses.end());

      RF_DBG_MSG("Stop buffering and build new LEI region." << endl);

      if (recording_buffer.addresses.size() > 0) {
        recorded[recording_buffer.addresses[0]] = true;
        history_buffer.addresses.erase(
            history_buffer.addresses.end()-recording_buffer.addresses.size(),
            history_buffer.addresses.end());
      }

      buildRegion();
    }
    recording = false;
  }

  last_addr = cur_addr;
}
