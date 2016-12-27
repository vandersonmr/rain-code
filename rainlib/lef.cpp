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
#include <algorithm>
#include <cstring>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

bool LEF::isRetInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 195 || opcode == 194 || opcode == 202 || opcode == 203);
}

bool LEF::isCallInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 232 || opcode == 154);
}

void LEF::updateOutAddrs(rain::Region* reg, pair_addr e) { // edge == pair<ull, ull>
  if (reg_out_addrs.count(reg) == 0) {
    shared_ptr<set<pair_addr>> tgt_addrs(new set<pair_addr>);
    tgt_addrs->insert(e);
    reg_out_addrs.insert(make_pair(reg, tgt_addrs));
  } else {
    reg_out_addrs[reg]->insert(e);
  }
}

void LEF::mergeRegions(rain::Region* src_reg, unsigned long long src_addr,
    rain::Region* tgt_reg, unsigned long long tgt_addr) {

    tgt_reg->moveAndDestroy(src_reg, rain.region_entry_nodes);

    // Check if already exist an edge
    rain::Region::Edge* ed = tgt_reg->getNode(src_addr)->findOutEdge(tgt_addr);
    if (ed) {
      tgt_reg->region_inner_edges.insert(ed);
      tgt_reg->exit_nodes.erase(tgt_reg->getNode(src_addr));
    } else {
      tgt_reg->createInnerRegionEdge(tgt_reg->getNode(src_addr),
                                     tgt_reg->getNode(tgt_addr));
    }

    rain.regions.erase(src_reg->id);
    rain.region_entry_nodes.erase(tgt_addr);
}

bool LEF::hasComeFromCall(rain::Region* reg) {
  return came_from_call.count(reg) != 0;
}

void LEF::insertEntryAddrs(rain::Region* reg, vector<unsigned long long>& entry_addrs) {
  for (auto node : reg->entry_nodes)
    entry_addrs.push_back(node->getAddress());
}

void LEF::expandRegion(rain::Region* reg) {
  if (hasComeFromCall(reg))
    return;

  vector<unsigned long long> entry_addrs;
  insertEntryAddrs(reg, entry_addrs);

  bool newNeighbors = true;
  while (newNeighbors) {
    newNeighbors = false;
    vector<unsigned long long> entry_addrs_aux;
    // Iterate over every region
    for (auto pair : reg_out_addrs) {

      // Check if the region is alive
      if (!pair.first->alive) continue;

      // Iterate over all out edges from the region
      for (auto edge : *pair.second) {
        // If the out edge ends in any of the region entry addrs which is being 
        // expanded, then both of them are merged.
        auto it = find(entry_addrs.begin(), entry_addrs.end(), edge.second);
        if (it != entry_addrs.end()) {
          unsigned long long src_addr = edge.first;
          unsigned long long tgt_addr = edge.second;
          rain::Region* src_reg = pair.first;
          rain::Region* tgt_reg = nullptr;

          if (rain.region_entry_nodes.count(tgt_addr) != 0)
            tgt_reg = rain.region_entry_nodes[tgt_addr]->region;

          if (hasComeFromCall(src_reg))
            return;

          if (tgt_reg != nullptr) {
            insertEntryAddrs(src_reg, entry_addrs_aux);
            mergeRegions(src_reg, src_addr, tgt_reg, tgt_addr);
          }

          newNeighbors = true;
        }
      }
    }

    entry_addrs.clear();
    entry_addrs = entry_addrs_aux;
  }

  // Check if all inner loops are correctly seted
  for (auto node : reg->nodes) {
    for (auto edge = node->in_edges; edge != NULL; edge = edge->next) {
      Region::Edge* ed = edge->edge;
      if (ed->src->region == ed->tgt->region && ed->src->region != NULL) { 
        Region* r = ed->src->region;
        r->region_inner_edges.insert(ed);
      }
    }
  }
}

void LEF::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
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

    updateOutAddrs(edg->src->region, make_pair(edg->src->getAddress(), cur_addr));
  }
  else if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
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
      retRegion = false;
      callRegion = false;
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
    else if (isRetInst(cur_opcode)) {
      stopRecording = true;
    }
    else if (recording_buffer.contains_address(cur_addr)) {
      // Hit an instruction already recorded (loop)
      RF_DBG_MSG("Stopped recording because isnt " << "0x" << setbase(16) << 
          cur_addr << " is already included on the recording buffer." << endl);
      stopRecording = true;
    }
    else if (recording_buffer.addresses.size() > 1) {
      if (cur_addr < last_addr) {
        stopRecording = true;
      }
      // Only check if buffer alreay has more than one instruction recorded.
      else if (switched_mode(recording_buffer.addresses.back(), cur_addr)) {
        if (!mix_usr_sys) {
          // switched between user and system mode
          RF_DBG_MSG("Stopped recording because processor switched mode: 0x" << setbase(16) << 
              last_addr << " -> 0x" << cur_addr << endl);
          stopRecording = true;
        }
      }
    }

    if (isRetInst(cur_opcode))
      retRegion = true;

    if (isCallInst(last_opcode))
      callRegion = true;

    if (stopRecording) {
      // Create region and add to RAIn TEA
      // merge both -> save to recording
      RF_DBG_MSG("Stop buffering and build new LEF region." << endl);
      recording = false;
      rain::Region* r = buildRegion();
      if (r != nullptr) {
        if (callRegion)
          came_from_call[r] = true;
        if (retRegion)
          expandRegion(r);
      }
    }
    else {
      // Record target instruction on region formation buffer
      RF_DBG_MSG("Recording " << "0x" << setbase(16) <<
          cur_addr << " on the recording buffer" << endl);
      recording_buffer.append(cur_addr); //, cur_opcode, cur_length);
    }
  }

  strncpy(last_opcode, cur_opcode, 16);
  last_addr = cur_addr;
}

void LEF::finish() {

}
