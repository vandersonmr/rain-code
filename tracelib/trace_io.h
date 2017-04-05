/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   - Edson Borin (edson@ic.unicamp.br)                                   *
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
#ifndef TRACE_IO_H
#define TRACE_IO_H

#include <string>

using namespace std;

namespace trace_io {
  
  /** The trace contains a sequence of trace items, which are classified in three
   *  types: 
   *  0 -- memory read address
   *  1 -- memory store address
   *  2 -- instruction
   * 
   *  Instruction items may (or may not) be followed by memory addresses items
   *  (type 0 or 1). These items indicate the memory address accessed by the
   *  instruction before it.
   */

  struct trace_item_t
  {
    char               type;
    unsigned long long addr;
    char               opcode[16];
    unsigned char      length;
    unsigned char      mem_size;

    bool is_mem_read() { return (type == 0); }
    bool is_mem_write() { return (type == 1); }
    bool is_instruction() { return (type == 2); }
  };

  class input_pipe_t
  {
  public:
    /** Gets the next item on the trace. Returns true if the item was retrieved,
	false if there are no more items. */
    virtual bool get_next_item(trace_item_t& item) = 0;

    /** Gets the next instruction from the trace. Similar to get_next_item, but
	ignores non-instruction items. */
    virtual bool get_next_instruction(trace_item_t& item) = 0;
  };

  class output_pipe_t
  {
  public:
    /** Writes the next item to the trace. */
    virtual void write_trace_item(trace_item_t& item) = 0;
  };

  class raw_input_pipe_t : public input_pipe_t {
  public:

    /** Constructor */
  raw_input_pipe_t(const string& b, int s_idx, int e_idx) : 
    basename(b), start_idx(s_idx), end_idx(e_idx), curr_idx(s_idx), current_fh(NULL)
    {};
    
    /** Destructor */
    ~raw_input_pipe_t();

    /** Gets the next item on the trace. Returns true if the item was retrieved,
	false if there are no more items. */
    bool get_next_item(trace_item_t& item);

    /** Gets the next instruction from the trace. Similar to get_next_item, but
	ignores non-instruction items. */
    bool get_next_instruction(trace_item_t& item) {
      do {
	if (!get_next_item(item)) 
	  return false;
      } while (!item.is_instruction());
      return true;
    }
    
  private:
    string basename;
    int start_idx;
    int end_idx;
    
    // Current index;
    int curr_idx;
    // Current file handler
    FILE* current_fh;

    string sys_cmd;
  };

  class raw_output_pipe_t : public output_pipe_t 
  {
  public:

    /** Constructor */
  raw_output_pipe_t(const string& out_filename) : 
    basename(out_filename), fh(NULL)
    {};
    
    /** Destructor */
    ~raw_output_pipe_t();

    /** Writes the next item to the trace. */
    void write_trace_item(trace_item_t& item);
    
  private:
    string basename;
    // Current file handler
    FILE* fh;

    string sys_cmd;
  };

  
};

#endif  // TRACE_IO_H
