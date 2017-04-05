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
#include <elfio/elfio.hpp>

#include <trace_io.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

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
unordered_map<unsigned long, vector<trace_io::trace_item_t>> traces;

int main(int argc, char **argv, char **envp) {
  string sAddrs, sOpcode, sLength;
  int k = 0;
  unsigned long long processedInstructions = 1;
  while (true) {
out:
    if (k >= 100) break;
    std::cout << "Creating out" << k << "\n";
    out = new trace_io::raw_output_pipe_t(string("out.")+std::to_string(k++));
    string line;
    int tracecounter = 0;
    while (getline(std::cin, line)) {
      if (line[0] == '@') { 
        std::istringstream iss(line);
        char trash;
        iss >> trash >> tracecounter;
        traces[tracecounter];
      } else if (line[0] == '0') {
        std::istringstream iss(line);

        trace_io::trace_item_t t;
        t.type = 2;

        getline(iss, sLength, '|');
        istringstream isLength(sLength);
        isLength >> t.length;
        if (t.length == 0) {
          std::cout << "Empty length " << tracecounter << std::endl;
          return 1;
        }

        getline(iss, sAddrs, '|');
        istringstream isAddrs(sAddrs);
        isAddrs >> hex >> t.addr;
        if (t.addr == 0) {
          t.addr = 1;
          std::cout << "Warning: more than one address "<< tracecounter << "\n";
        }

        getline(iss, sOpcode, '\n');
        istringstream isOpcode(sOpcode);
        int i = 0;
        unsigned tmp;
        while (isOpcode >> hex >> tmp) t.opcode[i++] = tmp;

        traces[tracecounter].push_back(t);
      } else {
        istringstream traceNum(line);
        unsigned long traceId;
        traceNum >> traceId;
        for (auto ins : traces[traceId]) {
          processedInstructions++;
          out->write_trace_item(ins);
        }
        if ((processedInstructions % 1000000) == 0) {
          float p = (((float)processedInstructions / 1E10)*100); 
          std::cout << p << "%" << k << std::endl;
          if (p >= k) {
            std::cout << "Finished out."<< k << "\n";
            delete out;
            goto out;
          }
        }
      }
    }
    delete out;
    break;
  }

  return 0;
}
