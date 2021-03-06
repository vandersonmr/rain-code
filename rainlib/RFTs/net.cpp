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
#include <iomanip>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

void NET::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length) {
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg)
    edg = rain.addNext(cur_addr);
  rain.executeEdge(edg);

  RF_DBG_MSG("0x" << setbase(16) << cur_addr << endl);

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg->src->region != NULL) && edg->tgt == rain.nte) {
    // Exits from previously selected hot traces are treated as start-of-trace points
    profile_target_instr = true;
  } if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording) {
      // Start region formation....
      RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);
      recording_buffer.reset();
      recording = true;
    }
  }

  if (recording) {
    // Check for stop conditions.
    // DBG_ASSERT(edg->src == rain.nte);
    bool stopRecording = false;
    if (edg->tgt != rain.nte) {
      // Found region entry
      RF_DBG_MSG("Stopped recording because found a region entry." << endl);
      stopRecording = true;
    } else if (recording_buffer.addresses.size() > MAX_INST_REG) {
      stopRecording = true;
    } else if (recording_buffer.addresses.size() > 1 && cur_addr < last_addr) {
      stopRecording = true;
    }

    if (stopRecording) {
      // Create region and add to RAIn TEA
      // merge both -> save to recording
      RF_DBG_MSG("Stop buffering and build new NET region." << endl);
      recording = false;
      buildRegion();
    } else {
      // Only add a new instruction if it is from the same type 
      // as the first one in the recording buffer
      if (is_region_addr_space(cur_addr)) {
        // Record target instruction on region formation buffer
        RF_DBG_MSG("Recording " << "0x" << setbase(16) <<
            cur_addr << " on the recording buffer" << endl);
        recording_buffer.append(cur_addr); //, cur_opcode, cur_length);
      }
    }
  }

  last_addr = cur_addr;
}
