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
  return (opcode == 0xc3 || opcode == 0xcb);
}

bool LEF::isCallInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 0xe8 || opcode == 0xff || opcode == 0x9a);
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

void LEF::
mergeRegions(rain::Region* src_reg, unsigned long long src_addr, rain::Region* tgt_reg, unsigned long long tgt_addr) {
    src_reg->exit_nodes.clear();
    for (auto r : tgt_reg->entry_nodes)
      rain.region_entry_nodes.erase(r->getAddress());
    tgt_reg->entry_nodes.clear();

    tgt_reg->moveAndDestroy(src_reg, rain.region_entry_nodes);
    src_reg->alive = false;
    rain.regions.erase(src_reg->id);
}

bool LEF::hasComeFromCall(rain::Region* reg) {
  return came_from_call.count(reg) != 0;
}

void LEF::expandRegion(rain::Region* reg) {
  if (hasComeFromCall(reg)) 
    return;

  bool newNeighbors = true;

  while (newNeighbors) {
    nextNeighbor:
    newNeighbors = false;
    // Iterate over every region
    for (auto pair : reg_out_addrs) {
      // Check if the region is alive
      if (!pair.first->alive) continue;
      // Iterate over all out edges from the region
      for (auto edge : *pair.second) {
        unsigned long long src_addr = edge.first;
        unsigned long long tgt_addr = edge.second;
        // If the out edge ends in any of the region entry addrs which is being 
        // expanded, then both of them are merged.
        if (reg->entry_nodes.count(reg->getNode(tgt_addr))) {
          rain::Region* src_reg = pair.first;
          rain::Region* tgt_reg = reg;

          mergeRegions(src_reg, src_addr, tgt_reg, tgt_addr);

          // Check if all inner loops are correctly seted
          for (auto node : reg->nodes) {
            for (auto edge = node->in_edges; edge != NULL; edge = edge->next) {
              Region::Edge* ed = edge->edge;

              if (ed->src->region == ed->tgt->region && ed->src->region != NULL) {
                Region* r = ed->src->region;
                r->region_inner_edges.insert(ed);

                if (r->entry_nodes.count(ed->tgt) != 0) {
                  r->entry_nodes.erase(ed->tgt);
                  rain.region_entry_nodes.erase(ed->tgt->getAddress());
                }
              }
            }
          }

          newNeighbors = true;

          if (hasComeFromCall(src_reg)) {
            if (reg->getNode(came_from_call[src_reg]) != NULL)
              reg->setEntryNode(reg->getNode(came_from_call[src_reg]));
            goto exit;
          }
        }
      }
    }
  }

  exit:
  rain.countExpansion();
}

void LEF::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length) {
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg)
    edg = rain.addNext(cur_addr);
  rain.executeEdge(edg);

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg->src->region != NULL) && edg->tgt == rain.nte) {
    // Profile NTE instructions that are target of regions instructions
    // Region exits
    profile_target_instr = true;

    updateOutAddrs(edg->src->region, make_pair(edg->src->getAddress(), cur_addr));
  } else if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording) {
      // Start region formation....
      recording_buffer.reset();
      recording = true;
      retRegion = 0;
      callRegion = 0;
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

    if (isRetInst(cur_opcode))
      retRegion = cur_addr;

    if (recording_buffer.addresses.size() == 0)
      if (isCallInst(last_opcode))
        callRegion = cur_addr;

    if (stopRecording) {
      // Create region and add to RAIn TEA
      // merge both -> save to recording
      RF_DBG_MSG("Stop buffering and build new LEF region." << endl);
      recording = false;
      rain::Region* r = buildRegion();
      if (r != nullptr) {
        if (callRegion != 0)
          came_from_call[r] = callRegion;

        if (retRegion != 0) {
          expandRegion(r);
          if (r->getNode(retRegion) != NULL)
            r->setExitNode(r->getNode(retRegion));
        }
      }
    }
    else {
      if (is_region_addr_space(cur_addr)) {
        // Record target instruction on region formation buffer
        RF_DBG_MSG("Recording " << "0x" << setbase(16) <<
            cur_addr << " on the recording buffer" << endl);
        recording_buffer.append(cur_addr); //, cur_opcode, cur_length);
      }
    }
  }

  strncpy(last_opcode, cur_opcode, 16);
  last_addr = cur_addr;
}
