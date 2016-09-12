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
#ifndef RF_TECHNIQUES_H
#define RF_TECHNIQUES_H

#include "rain.h"
#include <unordered_map>

#include <vector>
#include <deque>

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

#define HOT_THRESHOLD 25
  static unsigned long long system_threshold;

  /** Instruction hotness profiler. */
  struct profiler_t {
    map<unsigned long long, unsigned long long> instr_freq_counter;

    /** Update profile information. */
    void update(unsigned long long addr) {
      map<unsigned long long, unsigned long long>::iterator it = 
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
      map<unsigned long long, unsigned long long>::iterator it = 
        instr_freq_counter.find(addr);
      assert(it != instr_freq_counter.end() && "Trying to reset a header (addr) that doesn't exist!");
      it->second = 3;
    }

    /** Check whether instruction is already hot. */
    bool is_hot(unsigned long long addr) {
      map<unsigned long long, unsigned long long>::iterator it = 
        instr_freq_counter.find(addr);
      if (it != instr_freq_counter.end())
        return (it->second >= HOT_THRESHOLD);
      else
        return false;
    }
  };

  /** Buffer to record new regions. */
  struct recording_buffer_t {

    /** List of instruction addresses. */
    vector<unsigned long long> addresses;

    void reset() { addresses.clear(); }

    void append(unsigned long long addr) { addresses.push_back(addr); }

    bool contains_address(unsigned long long addr)
    {
      vector<unsigned long long>::iterator it;
      for (it = addresses.begin(); it != addresses.end(); it++) {
        if ( (*it) == addr) 
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

    virtual void finish() = 0;

    rain::RAIn rain;

    static void set_system_threshold(unsigned long long addr) {
      system_threshold = addr;
    }

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
      return (is_system_instr(src) != is_system_instr(tgt));
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

      vector<unsigned long long>::iterator it;
      for (it = recording_buffer.addresses.begin();
           it != recording_buffer.addresses.end(); it++) {
        unsigned long long addr = (*it);

        rain::Region::Node* node = new rain::Region::Node(addr);
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

    void finish();

  private:

    bool recording;
    unsigned long long last_addr;

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the Last Executing Function (LEF) region formation
   * technique.
   */
  class LEF : public RF_Technique
  {
  public:

    LEF() : recording(false), merging(false), last_addr (0)
    { std::cout << "Initing LEF\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish();

  private:

    bool merging;
    bool recording;
    unsigned long long last_addr;
    
    map<unsigned long long, bool> region_with_ret;

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the Last-Executed Iteration (LEI) region formation
   * technique.
   */
  class LEI : public RF_Technique
  {
  public:

    LEI() : recording(false), last_addr(0)
    { std::cout << "Initing LEI\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish() {};

  private:

    bool recording;
    unsigned long long last_addr;
    recording_buffer_t history_buffer;
    unordered_map<unsigned long long, bool> recorded;

    bool hasRecorded(unsigned long long addr);

    using RF_Technique::buildRegion;
  };

  /** 
   * Class to evaluate the Last-Executed Iteration (LEI) region formation
   * technique.
   */
  class MRET2 : public RF_Technique
  {
  public:

    MRET2() : recording(false), last_addr(0), stored_index(0)
    { std::cout << "Initing MRET2\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish() {};

  private:

    bool recording;
    unsigned long long last_addr;
    unsigned long long header;
    unordered_map<unsigned long long, unsigned> phases;
    unordered_map<unsigned long long, bool> recorded;

    recording_buffer_t recording_buffer_tmp;

    unsigned int stored_index;
    recording_buffer_t stored[5];

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
    #define LIMIT_SIDE_NODE_BRANCH 990

    TraceTree() : recording(false), last_addr(0), is_side_exit(false)
    { std::cout << "Initing TraceTree\n" << std::endl; }

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish() {};

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
