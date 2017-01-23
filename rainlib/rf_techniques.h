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
#ifndef RF_TECHNIQUES_H
#define RF_TECHNIQUES_H

#include "rain.h"
#include "arglib.h"

#include <unordered_map>
#include <map>
#include <vector>
#include <set>
#include <deque>
#include <memory>
#include <algorithm>

#include <cassert>
#include <iostream> // cerr

//#define DEBUG_MSGS
#ifdef DEBUG_MSGS
#include <iomanip>  // setbase
#define RF_DBG_MSG(msg) std::cerr << msg
#else
#define RF_DBG_MSG(msg)
#endif

namespace rf_technique {
  static unsigned long long system_threshold = 0xB2D05E00; // FIXME
  static bool mix_usr_sys = false;

  class InstructionSet {
    map<unsigned long long, char[16]> instructions;
  public:

    map<unsigned long long, char[16]>::const_iterator find(unsigned long long addrs) const {
      return instructions.find(addrs);
    }

    map<unsigned long long, char[16]>::const_iterator getEnd() const {
      return instructions.end();
    }

    const char* getOpcode(unsigned long long addrs) const {
      return instructions.at(addrs);
    }

    bool hasInstruction(unsigned long long addrs) const {
      return instructions.count(addrs) != 0;
    }

    void addInstruction(unsigned long long addrs, char opcode[16]) {
      for (int i = 0; i < 16; i++)
        instructions[addrs][i] = opcode[i];
    }

    size_t size() {
      return instructions.size();
    }
  };
  /** Instruction hotness profiler. */
  struct profiler_t {
    profiler_t() : hot_threshold(50) {};
    unordered_map<unsigned long long, unsigned long long> instr_freq_counter;

    /** Update profile information. */
    void update(unsigned long long addr) {
      unordered_map<unsigned long long, unsigned long long>::iterator it =
        instr_freq_counter.find(addr);
      if (it == instr_freq_counter.end()) {
        RF_DBG_MSG("profiling: freq[" << "0x" << std::setbase(16) << addr << "] = 1" << endl);
        instr_freq_counter[addr] = 1;
      }
      else {
        it->second++;
        RF_DBG_MSG("profiling: freq[" << "0x" << std::setbase(16) << addr << "] = " << it->second << endl);
      }
    }

    void reset(unsigned long long addr) {
      unordered_map<unsigned long long, unsigned long long>::iterator it = 
        instr_freq_counter.find(addr);
      assert(it != instr_freq_counter.end() && "Trying to reset a header (addr) that doesn't exist!");
      it->second = 3;
    }

    /** Check whether instruction is already hot. */
    bool is_hot(unsigned long long addr) {
      unordered_map<unsigned long long, unsigned long long>::iterator it = 
        instr_freq_counter.find(addr);
      if (it != instr_freq_counter.end())
        return (it->second >= hot_threshold);
      else
        return false;
    }

    void set_hot_threshold(unsigned threshold) {
      hot_threshold = threshold;
    }

    unsigned getNumOfCounters() {
      return instr_freq_counter.size();
    }

  private:
    unsigned hot_threshold;
  };

  /** Buffer to record new regions. */
  struct recording_buffer_t {

    /** List of instruction addresses. */
    vector<unsigned long long> addresses;

    void reset() { addresses.clear(); }

    void reverse() { std::reverse(addresses.begin(), addresses.end()); }

    void append(unsigned long long addr) { addresses.push_back(addr); }

    bool contains_address(unsigned long long addr)
    {
      for (auto I : addresses) {
        if (I == addr)
          return true;
      }
      return false;
    }
  };

  class RF_Technique
  {
  public:
    virtual void 
      process(unsigned long long cur_addr, char cur_opcode[16], 
          char unsigned cur_length,
          unsigned long long nxt_addr, char nxt_opcode[16], 
          char unsigned nxt_length) = 0;

    void finish() {
      rain.setNumOfCounters(profiler.getNumOfCounters());
    };

    rain::RAIn rain;

    static void set_system_threshold(unsigned long long addr) {
      system_threshold = addr;
    }

    static void set_mix_usr_sys(bool mix) {
      mix_usr_sys = mix;
    }

    bool pauseRecording = false;

  protected:
    bool is_system_instr(unsigned long long addr)
    {
      return (addr >= system_threshold);
    }

    bool switched_mode(rain::Region::Edge* edg)
    {
      return switched_mode(edg->src->getAddress(), edg->tgt->getAddress());
    }

    bool switched_mode(unsigned long long src, unsigned long long tgt)
    {
      //return false;
      return (is_system_instr(src) != is_system_instr(tgt));
    }

    bool is_region_addr_space(unsigned long long tgt) {
      return recording_buffer.addresses.size() == 0 ||
        (!switched_mode(recording_buffer.addresses.front(), tgt) || mix_usr_sys);
    }

    profiler_t profiler;
    recording_buffer_t recording_buffer;

    rain::Region* buildRegion()
    {
      if (recording_buffer.addresses.size() == 0) {
        RF_DBG_MSG("WARNING: buildNETRegion() invoked, but recording_buffer is empty..." << endl);
        return NULL;
      }

      rain::Region* r = rain.createRegion();
      rain::Region::Node* last_node = NULL;

      for (auto addr : recording_buffer.addresses) {

        rain::Region::Node* node = new rain::Region::Node(addr);
        rain.insertNodeInRegion(node, r);

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

      return r;
    }
  };

  /** 
   * Class to evaluate the Next Executing Tail (NET) region formation
   * technique.
   */
  class NET : public RF_Technique
  {
  public:

    NET() : recording(false), last_addr (0)
    { std::cout << "Initing NET\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:

    bool recording;
    unsigned long long last_addr;

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the NETPlus (NET+) region formation
   * technique.
   */
  class NETPlus : public RF_Technique
  {
  public:

    NETPlus(InstructionSet& inst, unsigned limit)
      : recording(false), last_addr(0), instructions(inst), DEPTH_LIMIT(limit)
    { std::cout << "Initing NETPlus ("<< DEPTH_LIMIT << ")\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:

    unsigned DEPTH_LIMIT = 10;

    bool recording;
    unsigned long long last_addr;

    InstructionSet& instructions;

    void addNewPath(rain::Region*, recording_buffer_t&);
    void expand(rain::Region*);

    using RF_Technique::buildRegion;
  };


  /**
   * Class to evaluate the Last Executing Function (LEF) region formation
   * technique
   */
  class LEF : public RF_Technique
  {
  public:

    LEF() : recording(false), retRegion(true), callRegion(true), last_addr (0)
    { std::cout << "Initing LEF\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:
    typedef pair<unsigned long long, unsigned long long> pair_addr;
    typedef shared_ptr<set<pair_addr>> set_addr_uptr;

    bool isRetInst(char[16]);
    bool isCallInst(char[16]);
    void updateOutAddrs(rain::Region*, pair_addr);

    void mergeRegions(rain::Region*, unsigned long long, 
                     rain::Region*, unsigned long long);
    bool hasComeFromCall(rain::Region*);
    void expandRegion(rain::Region*);

    bool recording, retRegion, callRegion;
    unsigned long long last_addr;

    char last_opcode[16];

    unordered_map<rain::Region*, set_addr_uptr> reg_out_addrs;
    unordered_map<rain::Region*, bool> came_from_call;

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the Last-Executed Iteration (LEI) region formation
   * technique.
   */
  class LEI : public RF_Technique
  {
  public:

    LEI(InstructionSet& inst)
      : recording(false), last_addr(0), instructions(inst)
    { std::cout << "Initing LEI\n" << std::endl; profiler.set_hot_threshold(35); }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:

    bool recording;
    unsigned long long last_addr;
    unordered_map<unsigned long long, bool> recorded;

    InstructionSet& instructions;

    #define MAX_SIZE_BUFFER 100000
    int buf_top = 0;

    struct branch_t {
      unsigned long long src;
      unsigned long long tgt;
      rain::Region::Edge* edge;
    };

    rain::Region::Node* insertNode(rain::Region*, rain::Region::Node*, unsigned long long);

    std::vector<branch_t> buf;
    unordered_map<unsigned long long, int> buf_hash;
    unordered_map<unsigned long long, bool> code_cache;
    void circularBufferInsert(unsigned long long, unsigned long long, rain::Region::Edge*);
    bool is_followed_by_exit(int);
    void formTrace(unsigned long long, int);

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the Most Recent Executed Trace (MRET2) region formation
   * technique.
   */
  class MRET2 : public RF_Technique
  {
  public:
    #define STORE_INDEX_SIZE 10000
    #define MAX_INST_REG 1000


    MRET2() : recording(false), last_addr(0), stored_index(0)
    { std::cout << "Initing MRET2\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:

    bool recording;
    unsigned long long last_addr;
    unsigned long long header;
    unordered_map<unsigned long long, unsigned> phases;
    unordered_map<unsigned long long, bool> recorded;

    recording_buffer_t recording_buffer_tmp;

    unsigned int stored_index;
    recording_buffer_t stored[STORE_INDEX_SIZE];

    using RF_Technique::buildRegion;

    void mergePhases();
    unsigned int getStoredIndex(unsigned long long addr);
    unsigned getPhase(unsigned long long addr);
    bool hasRecorded(unsigned long long addr);
  };

  /** 
   * Class to evaluate the TraceTree (TT) region formation
   * technique.
   */
  class TraceTree : public RF_Technique
  {
  public:
    #define TREE_SIZE_LIMIT 1000
    #define BACK_BRANCH_LIMIT 8

    TraceTree() : recording(false), last_addr(0), is_side_exit(false)
    { std::cout << "Initing TraceTree\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:

    bool is_side_exit;
    rain::Region* side_exit_region;
    rain::Region::Node* side_exit_node;

    bool recording;
    unsigned long long last_addr;
    int inner_loop_trial = 0;

    using RF_Technique::buildRegion;
    void expand(rain::Region::Node*);
  };
}; // namespace rf_technique

#endif  // RF_TECHNIQUES_H
