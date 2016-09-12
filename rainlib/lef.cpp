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

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

bool isRetInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 195 || opcode == 194 || opcode == 202 || opcode == 203);
}

bool isCallInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 232 || opcode == 154);
}

void LEF::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg) {
    edg = rain.addNext(cur_addr);
  }
  rain.executeEdge(edg);
  //!TODO if both tgt and src from edg are in regions and the tgt is a ret region
  // merge both regions in one

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
      if (region_with_ret.count(edg->tgt->getAddress()) != 0)
        merging = true;
    }
    else if (recording_buffer.contains_address(last_addr) && (cur_addr <= last_addr)) {
      stopRecording = true;
    }
    else if (isRetInst(cur_opcode)) {
      stopRecording = true;
      region_with_ret.insert(pair<unsigned long long, bool>(recording_buffer.addresses.front(), true));
    }
    else if (recording_buffer.contains_address(cur_addr)) {
      // Hit an instruction already recorded (loop)
      RF_DBG_MSG("Stopped recording because isnt " << "0x" << setbase(16) << 
          cur_addr << " is already included on the recording buffer." << endl);
      stopRecording = true;
    }
    else if (recording_buffer.addresses.size() > 1) {
      // Only check if buffer alreay has more than one instruction recorded.
      if (switched_mode(recording_buffer.addresses.back(), cur_addr)) {
//        if (!mix_usr_sys.was_set()) {
          // switched between user and system mode
          RF_DBG_MSG("Stopped recording because processor switched mode: 0x" << setbase(16) << 
              last_addr << " -> 0x" << cur_addr << endl);
          stopRecording = true;
//        }
      }
    }

    if (stopRecording) {
        // Create region and add to RAIn TEA
        // merge both -> save to recording
        RF_DBG_MSG("Stop buffering and build new LEF region." << endl);
        recording = false;
        rain::Region* r = buildRegion();

        if (merging) {
          merging = false;
        }
    }
    else {
      // Record target instruction on region formation buffer
      RF_DBG_MSG("Recording " << "0x" << setbase(16) <<
          cur_addr << " on the recording buffer" << endl);
      recording_buffer.append(cur_addr); //, cur_opcode, cur_length);
    }
  }

  last_addr = cur_addr;
}

void LEF::finish() {

}
