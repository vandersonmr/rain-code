/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   Edson Borin (edson@ic.unicamp.br)                                     *
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

/**
 * See usage() function for a description.
 */

#include "arglib.h"
#include "trace_io.h"
#include <fstream> // ofstream

using namespace std;

clarg::argInt    start_i("-s", "start: first file index ", 0);
clarg::argInt    end_i("-e", "end: last file index", 0);
clarg::argString basename("-b", "input file basename", "trace");
clarg::argString out_fn("-ofn", "output file basename", "out_trace");
clarg::argBool   help("-h",  "display the help message");

void usage(char* prg_name) 
{
  cout << "Usage: " << prg_name << " -b basename -s index -e index [-h] -ofn output_filename" 
    << endl << endl;

  cout << "DESCRIPTION:" << endl;

  cout << "....." << endl;

  cout << "ARGUMENTS:" << endl;
  clarg::arguments_descriptions(cout, "  ", "\n");
}

int validate_arguments() 
{
  if (!start_i.was_set()) {
    cerr << "Error: you must provide the start file index."
      << "(use -h for help)" << endl;
    return 1;
  }

  if (!end_i.was_set()) {
    cerr << "Error: you must provide the end file index."
      << "(use -h for help)" << endl;
    return 1;
  }

  if (!basename.was_set()) {
    cerr << "Error: you must provide the basename."
      << "(use -h for help)" << endl;
    return 1;
  }

  if (end_i.get_value() < start_i.get_value()) {
    cerr << "Error: start index must be less (<) or equal (=) to end index" 
      << "(use -h for help)" << endl;
    return 1;
  }

  return 0;
}

int main(int argc,char** argv)
{
  // Parse the arguments
  if (clarg::parse_arguments(argc, argv)) {
    cerr << "Error when parsing the arguments!" << endl;
    return 1;
  }

  if (help.get_value() == true) {
    usage(argv[0]);
    return 1;
  }

  if (validate_arguments()) 
    return 1;

  // Create the input pipe.
  trace_io::raw_input_pipe_t in(basename.get_value(), 
      start_i.get_value(), 
      end_i.get_value());

  // Create the input pipe.
  trace_io::raw_output_pipe_t out(out_fn.get_value());

  trace_io::trace_item_t trace_item;

  // While there are instructions
  while (in.get_next_instruction(trace_item)) {
    // Clean unused bytes
    for(int i=trace_item.length; i<16; i++)
      trace_item.opcode[i] = 0;
    out.write_trace_item(trace_item);
  }

  return 0; // Return OK.
}
