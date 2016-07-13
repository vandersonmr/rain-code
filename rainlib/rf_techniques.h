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

//#define DEBUG_MSGS
#ifdef DEBUG_MSGS
#include <iostream> // cerr
#include <iomanip>  // setbase
#define RF_DBG_MSG(msg) std::cerr << msg
#else
#define RF_DBG_MSG(msg)
#endif


namespace rf_technique {
  static unsigned long long system_threshold;

  class RF_Technique
  {
  public:

#define HOT_THRESHOLD 50

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

      /** Check whether instruction is already hot. */
      bool is_hot(unsigned long long addr) {
        map<unsigned long long, unsigned long long>::iterator it = 
          instr_freq_counter.find(addr);
        if (it != instr_freq_counter.end())
          return (it->second >= HOT_THRESHOLD);
        else
          return false;
      }
    } profiler;

    /** Buffer to record new regions. */
    struct recording_buffer_t {

      /** List of instruction addresses. */
      list<unsigned long long> addresses;

      void reset() { addresses.clear(); }

      void append(unsigned long long addr) { addresses.push_back(addr); }

      bool contains_address(unsigned long long addr)
      {
        list<unsigned long long>::iterator it;
        for (it = addresses.begin(); it != addresses.end(); it++) {
          if ( (*it) == addr) 
            return true;
        }
        return false;
      }
    } recording_buffer;

    bool switched_mode(rain::Region::Edge* edg)
    {
      return switched_mode(edg->src->getAddress(), edg->tgt->getAddress());
    }

    bool switched_mode(unsigned long long src, unsigned long long tgt)
    {
      return (is_system_instr(src) != is_system_instr(tgt));
    }

    void buildRegion()
    {
      if (recording_buffer.addresses.size() == 0) {
        RF_DBG_MSG("WARNING: buildNETRegion() invoked, but recording_buffer is empty..." << endl);
        return;
      }

      rain::Region* r = rain.createRegion();
      rain::Region::Node* last_node = NULL;

      list<unsigned long long>::iterator it;
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

      //recording_buffer.reset();
    }
  };

  /** 
   * Class to evaluate the Next Executing Tail (NET) region formation
   * technique.
   */
  class NET : public RF_Technique
  {
  public:

    NET() : recording(false),last_addr (0)
    {}

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish();

  private:

    bool recording;
    unsigned long long last_addr;

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
    {}

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish() {};

  private:

    bool recording;
    unsigned long long last_addr;
    recording_buffer_t history_buffer;
    unordered_map<unsigned long long int, bool> all_nodes_recorded;

    using RF_Technique::buildRegion;
  };

}; // namespace rf_technique


#endif  // RF_TECHNIQUES_H
