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

clarg::argBool mix_usr_sys("-mix_NET",  "Allow user and system code in the same NET regions.");

void NET::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
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
    // Check for stop conditions.
    // DBG_ASSERT(edg->src == rain.nte);
    bool stopRecording = false;
    if (edg->tgt != rain.nte) {
      // Found region entry
      RF_DBG_MSG("Stopped recording because found a region entry." << endl);
      stopRecording = true;
    }
    else if (recording_buffer.contains_address(last_addr) && (cur_addr <= last_addr)) {
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
        if (!mix_usr_sys.was_set()) {
          // switched between user and system mode
          RF_DBG_MSG("Stopped recording because processor switched mode: 0x" << setbase(16) << 
              last_addr << " -> 0x" << cur_addr << endl);
          stopRecording = true;
        }
      }
    }

    if (stopRecording) {
      // Create region and add to RAIn TEA
      RF_DBG_MSG("Stop buffering and build new NET region." << endl);
      recording = false;
      buildRegion();
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


//rain.createEdge(rain.cur_node,rain.next_node);
//rain.createOrUpdateEdge(rain.nte,rain.next_node);
void NET::buildRegion()
{
  if (recording_buffer.addresses.size() == 0) {
    cout << "WARNING: buildNETRegion() invoked, but recording_buffer is empty..." << endl;
    return;
  }

  Region* r = rain.createRegion();
  Region::Node* last_node = NULL;

  list<unsigned long long>::iterator it;
  for (it = recording_buffer.addresses.begin(); 
      it != recording_buffer.addresses.end(); it++) {
    unsigned long long addr = (*it);

    Region::Node* node = new Region::Node(addr);
    r->insertNode(node);

    if (!last_node) {
      // First node
#ifdef DEBUG
      // Make sure there were no region associated with the entry address.
      assert(rain.region_entry_nodes.find(node->getAddress()) == 
          rain.region_entry_nodes.end());
#endif
      rain.setEntry(node);
    }
    else {
      // Successive nodes
      r->createInnerRegionEdge(last_node, node);
    }

    last_node = node;
  }
  if (last_node) {
    rain.setExit(last_node);
  }

  RF_DBG_MSG("Region " << r->id << " created. # nodes = " <<
      r->nodes.size() << endl);

  //recording_buffer.reset();
}

void NET::finish()
{
}
