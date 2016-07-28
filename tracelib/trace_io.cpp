/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   - Edson Borin (edson@ic.unicamp.br)                                   *
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
#include "trace_io.h"
#include <iostream>
#include <sstream>  // stringstream
#include <stdio.h>  // pclose
#include <stdlib.h> // exit
#include <thread>
#include <chrono>
#include <errno.h>
#include <cstring>

using namespace trace_io;
using namespace std;

raw_input_pipe_t::~raw_input_pipe_t()
{
  if (current_fh)
    pclose(current_fh);
};

/** Gets the next item on the trace. Returns false if there are no items to be
    read, return true otherwise. */
bool raw_input_pipe_t::get_next_item(trace_item_t& item)
{
start:
  if (curr_idx > end_idx) 
    return false; // no more items to read

  if (!current_fh) {
    // Open the current_fh.
    ostringstream str;
    str << "gzip -cd " << basename << "." << curr_idx << ".bin.gz";
    sys_cmd = str.str();
    cout << "opening: " << curr_idx << endl;
    while( (current_fh = popen(sys_cmd.c_str(), "r")) == NULL) {
      cerr << "Error: (" << strerror(errno) << ") could not open the input pipe (" << sys_cmd << ")." << endl;

      std::this_thread::sleep_for(std::chrono::seconds(100));
      // TODO: handle errors gracefully (raise exception...)
      //exit(1);
    }
  }

  if (fread(&item.type,sizeof(char),1,current_fh) == 0) {
    // Nothing read.
    if (ferror(current_fh) != 0) {
      // Error.
      cerr << "Error: unexpected error when reading trace item. "
	   << "fread (...) returned error!" << endl;
      exit(1);
    }
    
    if (feof(current_fh) != 0) {
      // Ok, end-of-file. Close current file, update the curr_idx and try again.
      pclose(current_fh);
      current_fh = NULL;
      curr_idx++;
      return get_next_item(item); 
    }
    
    cerr << "Error: fread returned zero and EOF is not set..." << endl;
    exit (1);
  }
  
  if (item.type != 2) {
    // memory address 
    if (fread(&item.addr,sizeof(unsigned long long),1,current_fh) != 1) {
      cerr << "Error: Could not read address field from trace item (type = " 
	   << item.type << ")." << endl; 
      exit(1);
    }
    return true;
  }
  else {
    if (fread(&item.addr,sizeof(unsigned long long),1,current_fh) != 1) {
      cerr << "Error: Could not read address field from instruction "
	   << "trace item (type = 2)." << endl;
      exit(1);
    }
    if (fread(&item.opcode,16*sizeof(char),1,current_fh) != 1) {
      cerr << "Error: Could not read opcode field from instruction "
	   << "trace item (type = 2)." << endl;
      exit(1);
    }
    if (fread(&item.length,sizeof(char),1,current_fh) != 1) {
      cerr << "Error: Could not read length field from instruction "
	   << "trace item (type = 2)." << endl;
      exit(1);
    }
    if (fread(&item.mem_size,sizeof(char),1,current_fh) != 1) {
      cerr << "Error: Could not read mem_size field from instruction "
	   << "trace item (type = 2)." << endl;
      exit(1);
    }
    return true;
  }
}

raw_output_pipe_t::~raw_output_pipe_t()
{
  if (fh)
    pclose(fh);
};

void raw_output_pipe_t::write_trace_item(trace_item_t& item)
{
  if (!fh) {
    
    // Open the current_fh.
    ostringstream str;
    str << "gzip > " << basename << ".bin.gz";
    sys_cmd = str.str();
    if ( (fh = popen(sys_cmd.c_str(), "w")) == NULL) {
      cerr << "Error: could not open the input pipe (" << sys_cmd << ")." << endl;
      // TODO: handle errors gracefully (raise exception...)
      exit(1);
    }
  }
  
  if (fwrite(&item.type,sizeof(char), 1, fh) != 1) {
      cerr << "Error: unexpected error when writing trace item. "
	   << "fwrite (...) returned error!" << endl;
      exit(1);
  }
  if (item.type != 2) {
    // memory address 
    if (fwrite(&item.addr,sizeof(unsigned long long),1,fh) != 1) {
      cerr << "Error: Could not write address field to trace file (type = " 
	   << item.type << ")." << endl; 
      exit(1);
    }
  }
  else {
    if (fwrite(&item.addr,sizeof(unsigned long long),1,fh) != 1) {
      cerr << "Error: Could not write address field to output "
	   << "trace (type = 2)." << endl;
      exit(1);
    }
    if (fwrite(&item.opcode,16*sizeof(char),1,fh) != 1) {
      cerr << "Error: Could not write opcode field to output "
	   << "trace (type = 2)." << endl;
      exit(1);
    }
    if (fwrite(&item.length,sizeof(char),1,fh) != 1) {
      cerr << "Error: Could not write length field to output "
	   << "trace (type = 2)." << endl;
      exit(1);
    }
    if (fwrite(&item.mem_size,sizeof(char),1,fh) != 1) {
      cerr << "Error: Could not write mem_size field to output "
	   << "trace (type = 2)." << endl;
      exit(1);
    }
  }
}
