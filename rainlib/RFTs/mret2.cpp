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
#include "arglib.h"

#include <iostream>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

void MRET2::mergePhases() {
  unsigned long long addr1, addr2, end1, end2;
  unsigned i = 0, j = 0;

  recording_buffer_t recording_buffer_aux;

  if(0 == recording_buffer.addresses.size() || 0 == recording_buffer_tmp.addresses.size()) {
    recording_buffer.reset();
    return;
  }

  addr1 = recording_buffer_tmp.addresses[i];
  addr2 = recording_buffer.addresses[j];

  end1 = recording_buffer_tmp.addresses.back();
  end2 = recording_buffer.addresses.back();

  while(addr1 != end1 && addr2 != end2) {
    if(addr1 == addr2) {
      recording_buffer_aux.append(recording_buffer_tmp.addresses[i]);
      i++;
      j++;
      addr1 = recording_buffer_tmp.addresses[i];
      addr2 = recording_buffer.addresses[j];
    } else {
      break;
    }
  }

  if(addr1 == addr2)
    recording_buffer_aux.append(recording_buffer_tmp.addresses[i]);

  recording_buffer.addresses = recording_buffer_aux.addresses;
}

unsigned int MRET2::getStoredIndex(unsigned long long addr) {
  for (int i = 0; i < STORE_INDEX_SIZE; i++) 
    if (stored[i].addresses.size() > 0 && stored[i].addresses[0] == addr) return i;
  return 0;
}

unsigned MRET2::getPhase(unsigned long long addr) {
  if (phases.count(addr) == 0) phases[addr] = 1;
  return phases[addr];
}

bool MRET2::hasRecorded(unsigned long long addr) {
  if (recorded.count(addr) == 0) return false;
  return recorded[addr];
}

void MRET2::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length,
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
    // Profile NTE instructions that are target of regions instructions
    // Region exits
    profile_target_instr = true;
  } else if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording && !hasRecorded(cur_addr)) {
      RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);
      // Start region formation....
      if (getPhase(cur_addr) == 1) {
        profiler.reset(cur_addr);
      }
      header = cur_addr;
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
    } else if (recording_buffer.addresses.size() > 1) {
      if (cur_addr < last_addr) {
        stopRecording = true;
      }
    }

    if (stopRecording) {
      RF_DBG_MSG("Stop buffering and build new NET region." << endl);
      if (getPhase(header) == 1) {
        stored[stored_index].addresses = recording_buffer.addresses;
        recording_buffer.reset();

        stored_index++;
        if (stored_index == STORE_INDEX_SIZE) stored_index = 0;

        phases[header] = 2;
      } else {
        // Create region and add to RAIn TEA
        recording_buffer_tmp.addresses = stored[getStoredIndex(header)].addresses;
        mergePhases();
        rain::Region* r = buildRegion();
        recording_buffer.reset();
        phases[header] = 1;
        recorded[header] = true;
      }
      recording = false;
    } else {
      if (is_region_addr_space(cur_addr))
        recording_buffer.append(cur_addr);
    }
  }

  last_addr = cur_addr;
}

