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
