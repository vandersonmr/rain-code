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
#ifndef RF_UTILS_H
#define RF_UTILS_H

#include "rain.h"
#include "arglib.h"

#include <unordered_map>
#include <map>
#include <vector>
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

namespace rf_utils {
}

namespace rf_technique {
  class InstructionSet {
  private:
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
      std::cout << "Setting hotness threshold to " << threshold << "\n";
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

    bool contains_address(unsigned long long addr) {
      for (auto I : addresses)
        if (I == addr)
          return true;
      return false;
    }
  };
}

#endif
