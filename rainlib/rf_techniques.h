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

  /** 
   * Class to evaluate the Next Executing Tail (NET) region formation
   * technique.
   */
  class NET : public rain::RF_Technique
  {
  public:

    NET() : recording(false),last_addr (0)
    {}

    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

    void finish();

  protected:

    bool recording;

    unsigned long long last_addr;

    /** Instruction hotness profiler. */
    #define HOT_THRESHOLD 50
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

    /** Buffer to record new NET regions. */
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

    void buildRegion();

    bool switched_mode(rain::Region::Edge* edg)
    {
      return switched_mode(edg->src->getAddress(), edg->tgt->getAddress());
    }

    bool switched_mode(unsigned long long src, unsigned long long tgt)
    {
      return (is_system_instr(src) != is_system_instr(tgt));
    }

  };

  /** 
   * Class to evaluate the Last-Executed Iteration (LEI) region formation
   * technique.
   */
  class LEI : public NET
  {
  public:
    void process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
        unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length);

  private:
    recording_buffer_t history_buffer;

    unordered_map<unsigned long long int, bool> all_nodes_recorded;
  };

}; // namespace rf_technique


#endif  // RF_TECHNIQUES_H
