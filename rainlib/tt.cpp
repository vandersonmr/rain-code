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

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

void TraceTree::expand(rain::Region::Node* header) {
  rain::Region::Node* last_node = side_exit_node;

  for (auto it = recording_buffer.addresses.begin();
      it != recording_buffer.addresses.end(); it++) {

    unsigned long long addr = (*it);

    rain::Region::Node* node = new rain::Region::Node(addr);
    side_exit_region->insertNode(node);

    side_exit_region->createInnerRegionEdge(last_node, node);
    last_node = node;
  }

  side_exit_region->createInnerRegionEdge(last_node, header);

  rain.countExpansion();
}

void TraceTree::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg) {
    edg = rain.addNext(cur_addr);
  }
  rain.executeEdge(edg);

  RF_DBG_MSG("0x" << setbase(16) << cur_addr << endl);

  if (edg->src->region != NULL && edg->tgt == rain.nte && !recording) {
    is_side_exit = true;
    side_exit_region = edg->src->region;
    side_exit_node = edg->src;
    recording_buffer.reset();
    recording = true;
  }
  else if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
    // Profile instructions to detect hot code
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording) {
      // Start region formation....
      RF_DBG_MSG("0x" << setbase(16) << cur_addr << " is hot. Start Region formation." << endl);
      recording_buffer.reset();
      recording = true;
    }
  }

  if (recording && is_side_exit) {
    bool outLimit = false;
    if (recording_buffer.addresses.size() > TREE_SIZE_LIMIT
        || inner_loop_trial > BACK_BRANCH_LIMIT) {
      is_side_exit = false;
      recording = false;
      outLimit = true;
      recording_buffer.addresses.clear();
      inner_loop_trial = 0;
    }

    // Try to expand!
    if (!outLimit) {
      bool isInHeader = false;
      rain::Region::Node* header = NULL;
      for (auto it : side_exit_region->entry_nodes) {
        if (it->getAddress() == cur_addr) {
          isInHeader = true;
          header = it;
          break;
        }
      }

      if (isInHeader)  {
        expand(header);

        is_side_exit = false;
        recording = false;
      }
      else {
        if (cur_addr > nxt_addr)
          inner_loop_trial += 1;
        // Record target instruction on region formation buffer
        RF_DBG_MSG("Recording " << "0x" << setbase(16) <<
            cur_addr << " on the recording buffer" << endl);
        recording_buffer.append(cur_addr); //, cur_opcode, cur_length);
      }
    }
  }
  else if (recording) {
    // Check for stop conditions.
    // DBG_ASSERT(edg->src == rain.nte);
    bool stopRecording = false;
    if (edg->tgt != rain.nte) {
      // Found region entry
      RF_DBG_MSG("Stopped recording because found a region entry." << endl);
      stopRecording = true;
    }
    else if (recording_buffer.contains_address(last_addr) && (cur_addr < last_addr)) {
      stopRecording = true;
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
        // switched between user and system mode
        if (!mix_usr_sys) {
          RF_DBG_MSG("Stopped recording because processor switched mode: 0x" << setbase(16) << 
              last_addr << " -> 0x" << cur_addr << endl);
          stopRecording = true;
        }
      }
    }

    if (stopRecording) {
      // Create region and add to RAIn TEA
      // merge both -> save to recording
      RF_DBG_MSG("Stop buffering and build new NET region." << endl);
      buildRegion();
      recording = false;
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
