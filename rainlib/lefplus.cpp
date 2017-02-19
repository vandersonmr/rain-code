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
#include <queue>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

union int32 {
  int i;
  char bytes[4];
};

vector<unsigned long long> 
getPossibleNextAddr(unsigned long long cur_addr, const char cur_opcode[16]) {
  int opcode  = (int) (unsigned char) cur_opcode[0];
  int opcode1 = (int) (unsigned char) cur_opcode[1];

  vector<unsigned long long> nextAddrs;
  if (opcode == 0xe8 || opcode == 0xe9) {  // Call and near JMP

    union int32 offset;
    offset.bytes[0] = cur_opcode[1];
    offset.bytes[1] = cur_opcode[2];
    offset.bytes[2] = cur_opcode[3];
    offset.bytes[3] = cur_opcode[4];

    nextAddrs.push_back(cur_addr+5+offset.i);
  } else if // Near branches 
    ( (opcode == 0x0f && opcode1 == 0x84)
      || (opcode == 0x0f && opcode1 == 0x88)
      || (opcode == 0x0f && opcode1 == 0x8c)
      || (opcode == 0x0f && opcode1 == 0x89)
      || (opcode == 0x0f && opcode1 == 0x85)
      || (opcode == 0x0f && opcode1 == 0x8e)
      || (opcode == 0x0f && opcode1 == 0x82)
      || (opcode == 0x0f && opcode1 == 0x8d)
      || (opcode == 0x0f && opcode1 == 0x8f)
    ) { // near

      union int32 offset;
      offset.bytes[0] = cur_opcode[2];
      offset.bytes[1] = cur_opcode[3];
      offset.bytes[2] = cur_opcode[4];
      offset.bytes[3] = cur_opcode[5];

      nextAddrs.push_back(cur_addr+6+offset.i);
    } else if // short branches
      (    opcode == 0xeb || opcode == 0x74 || opcode == 0x78 
           || opcode == 0x72 || opcode == 0x7d || opcode == 0x7f 
           || opcode == 0x7c || opcode == 0x79 || opcode == 0x75 || opcode == 0x7e) {

        nextAddrs.push_back(cur_addr+2+ (signed char) cur_opcode[1]);
      }

    // If it is not a jmp, call or ret
    if (opcode != 0xe8 && opcode != 0xe9 && opcode != 0xeb && opcode != 0xc3) {
      if (opcode == 0x0f)
        nextAddrs.push_back(cur_addr+6);
      else
        nextAddrs.push_back(cur_addr+2);
    }

    return nextAddrs;
}

bool isFlowControl(const char cur_opcode[16]) {
  int opcode  = (int) (unsigned char) cur_opcode[0];
  int opcode1 = (int) (unsigned char) cur_opcode[1];
  return (opcode == 0xe8) // call *
    //  || (opcode == 0xc3) // ret
    || (opcode == 0xe9) // jmp near *
    || (opcode == 0xeb) // jmp short
    || (opcode == 0x74) // je short
    || (opcode == 0x0f && opcode1 == 0x84) // je near *
    || (opcode == 0x78) // js short
    || (opcode == 0x0f && opcode1 == 0x88) // js near *
    || (opcode == 0x72) // jc short
    || (opcode == 0x0f && opcode1 == 0x82) // jc near *
    || (opcode == 0x7d) // jge short
    || (opcode == 0x0f && opcode1 == 0x8d) // jge near *
    || (opcode == 0x7f) // jg short
    || (opcode == 0x0f && opcode1 == 0x8f) // jg near *
    || (opcode == 0x75) // jne short
    || (opcode == 0x0f && opcode1 == 0x85) // jne near *
    || (opcode == 0x7c) // jl short
    || (opcode == 0x0f && opcode1 == 0x8c) // jl near *
    || (opcode == 0x7e) // jle short
    || (opcode == 0x0f && opcode1 == 0x8e) // jle near *
    || (opcode == 0x79) // jns short
    || (opcode == 0x0f && opcode1 == 0x89); // jns near *
}

void LEFPlus::addNewPath(rain::Region* r, recording_buffer_t& newpath) {
  newpath.reverse();

  if (newpath.addresses.size() == 0)
    return;

  rain.countExpansion();
  rain::Region::Node* last_node = NULL;

  for (auto addr : newpath.addresses) {
    if (last_node == NULL) {
      last_node = r->getNode(addr);
      continue;
    }

    rain::Region::Node* node = r->getNode(addr);
    if (node == NULL) {
      node = new rain::Region::Node(addr);
      rain.insertNodeInRegion(node, r);
      recording_buffer.append(addr);
    }

    // Successive nodes
    if (last_node->findOutEdge(addr) == NULL)
      r->createInnerRegionEdge(last_node, node);

    last_node = node;
  }
}

#define DEPTH_LIMIT 10

void LEFPlus::expand(std::vector<rain::Region*> regions) {
  set<unsigned long long> loop_entries;
  for (auto reg : regions)
    for (rain::Region::Node* node : reg->entry_nodes)
      loop_entries.insert(node->getAddress());

  for (auto r : regions) {
    std::queue<unsigned long long> s;
    unordered_map<unsigned long long, unsigned> distance;
    unordered_map<unsigned long long, unsigned long long> next, parent;

    // Init BFS frontier
    int addrs_space = -1;
    for (rain::Region::Node* node : r->nodes) {
      unsigned long long addrs = node->getAddress();

      if (addrs_space == -1)
        addrs_space = is_system_instr(addrs);

      if (isFlowControl(instructions.getOpcode(addrs))) {
        s.push(addrs);
        distance[addrs] = 0;
        parent[addrs] = 0;
      }
    }

    while (!s.empty()) {
      unsigned long long current = s.front();
      s.pop();

      if (distance[current] < DEPTH_LIMIT) {

        for (auto target : getPossibleNextAddr(current, instructions.getOpcode(current))) {
          if (parent.count(target) != 0) continue;
          parent[target] = current;
          // Iterate over all instructions between the target and the next branch
          auto it = instructions.find(target);
          while (it != instructions.getEnd()) {

            if (loop_entries.count(it->first) != 0 && distance[current] > 0) {
              recording_buffer_t newpath;
              unsigned long long begin = it->first;
              unsigned long long prev = target;
              while (true) {
                auto it = instructions.find(begin);
                while (true) {
                  if (it->first == prev) break;
                  newpath.append(it->first);
                  --it;
                }
                begin = parent[prev];
                prev  = next[begin];
                if (prev == 0) {
                  newpath.append(begin);
                  break;
                }
              }
              addNewPath(r, newpath);

              break;
            }

            if (addrs_space != is_system_instr(it->first) ||
                rain.region_entry_nodes.count(it->first) != 0) {
              break;
            }

            if (isFlowControl(it->second) && distance.count(it->first) == 0) {
              s.push(it->first);
              distance[it->first] = distance[current] + 1;
              next[it->first] = target;
              break;
            }

            ++it;
          }
        }
      }
    }
  }
}

bool LEFPlus::isCallInst(char cur_opcode[16]) {
  int opcode = (int) (unsigned char) cur_opcode[0];
  return (opcode == 0xe8 || opcode == 0xff);
}

void LEFPlus::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  // Execute TEA transition.
  Region::Edge* edg = rain.queryNext(cur_addr);
  if (!edg)
    edg = rain.addNext(cur_addr);
  rain.executeEdge(edg);

  if (!instructions.hasInstruction(cur_addr))
    instructions.addInstruction(cur_addr, cur_opcode);

  if (call_stack.size() != 0) {
    call_status call = call_stack.top();
    if (call.caller + call.call_inst_length == cur_addr) {
      if (call.is_hot) {
        expand(call.regions);
      }
      call_stack.pop();
    }
  }

  if (isCallInst(last_opcode) && callsTaken.count(last_addr) == 0) {
    callsTaken[last_addr] = true;
    call_stack.push({last_addr, cur_addr, last_length, false});
  }

  // Profile instructions to detect hot code
  bool profile_target_instr = false;
  if ((edg->src->region != NULL) && edg->tgt == rain.nte) {
    // Profile NTE instructions that are target of regions instructions
    // Region exits
    profile_target_instr = true;
  }
  else if ((edg == rain.nte_loop_edge) && (cur_addr < last_addr)) {
    // Profile NTE instructions that are target of backward jumps
    profile_target_instr = true;
  }

  if (profile_target_instr) {
    profiler.update(cur_addr);
    if (profiler.is_hot(cur_addr) && !recording) {
      // Start region formation....
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
    }
    else if (recording_buffer.addresses.size() > MAX_INST_REG) {
      stopRecording = true;
    }
    else if (recording_buffer.addresses.size() > 1 && cur_addr < last_addr) {
      stopRecording = true;
    }

    if (stopRecording) {
      // Create region and add to RAIn TEA
      // merge both -> save to recording
      RF_DBG_MSG("Stop buffering and build new LEF region." << endl);
      recording = false;
      rain::Region* r = buildRegion();

      if (call_stack.size() > 0) {
        call_stack.top().is_hot = true;
        call_stack.top().regions.push_back(r);
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
  last_length = cur_length;
}
