/***************************************************************************
 *   Copyright (C) 2016 by:                                                *
 *   Edson Borin (edson@ic.unicamp.br)                                     *
 *   Vanderson M. Rosario (vandersonmr2@gmail.com)                         *
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

#include <unistd.h>
#include <inttypes.h>
#include <unicorn/unicorn.h>
#include <elfio/elfio.hpp>

#include <trace_io.h>
#include <fstream>
#include <sstream>

#include <string.h>
#include <stdio.h>

using namespace std;
using namespace ELFIO;

trace_io::raw_output_pipe_t *out;

/*
 * This program reads a trace in a raw format from the stdin
 * and convert and generates a compacted version compatible 
 * with RAIn.
 *
 * The raw format:
 *    addrs | opcode | length \n
 */
int main(int argc, char **argv, char **envp) {
  out = new trace_io::raw_output_pipe_t(string("test"));

  string sAddrs, sOpcode, sLength;
  while (getline(std::cin, sAddrs, '|')) {
    trace_io::trace_item_t t;

    getline(std::cin, sOpcode, '|');
    getline(std::cin, sLength);

    istringstream isAddrs(sAddrs);
    istringstream isOpcode(sOpcode);
    istringstream isLength(sLength);

    t.type = 2;
    isAddrs >> hex >> t.addr;
    int i = 0;
    unsigned tmp;
    while (isOpcode >> hex >> tmp) t.opcode[i++] = tmp;
    isLength >> t.length;

    out->write_trace_item(t);
  }

  delete out;
  return 0;
}
