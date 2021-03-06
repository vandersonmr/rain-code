/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
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

/**
 * See usage() function for a description.
 */

#include "arglib.h"
#include "trace_io.h"
#include "rain.h"
#include "rf_techniques.h"
#include <elfio/elfio.hpp>
#include <fstream> // ofstream
#include <udis86.h>

using namespace std;
using namespace rain;
using namespace ELFIO;

clarg::argInt    start_i("-s", "start: first file index ", 0);
clarg::argInt    end_i("-e", "end: last file index", 0);
clarg::argString trace_path("-b", "input file trace_path", "trace");
clarg::argInt    rf_threshold("-hot", "technique hot threshold", 50);
clarg::argString bin_path("-bin", "input binary file path", "");
clarg::argInt    depth_limit("-d", "depth limit for NETPlus", 10);
clarg::argString technique("-t", "RF Technique", "net");
clarg::argBool   help("-h",  "display the help message");
clarg::argString reg_stats_fname("-reg_stats", 
    "file name to dump regions statistics in CSV format", 
    "reg_stats.csv");
clarg::argString overall_stats_fname("-overall_stats", 
    "file name to dump overall statistics in CSV format", 
    "overall_stats.csv");
clarg::argBool mix_usr_sys("-mix",  "Allow user and system code in the same regions.");
clarg::argBool only_user("-only_user",  "Only allow user code to be emulated.");

#define LINUX_SYS_THRESHOLD   0xB2D05E00         // 3000000000
#define WINDOWS_SYS_THRESHOLD 0xF9CCD8A1C5080000 // 18000000000000000000 
#define STR_VALUE(arg) #arg

clarg::argBool lt("-lt", "linux trace. System/user address threshold = 0xB2D05E00");
clarg::argBool wt("-wt", "windows trace. System/user address threshold = 0xF9CCD8A1C5080000");

void usage(char* prg_name) {
  cout << "Version: 1.0.0 (02-21-2017)" << endl << endl;

  cout << "Usage: " << prg_name << 
    " -b trace_path -s index -e index [-h] [-o stats.csv] [-mix] {-lt|-wt} [-bin path/binary] \n\n";

  cout << "DESCRIPTION:\n";

  cout << "This program implements the RAIn (Region Appraisal Infrastructure) and can be\n";
  cout << "used to investigate region formation strategies for dynamic binary\n";
  cout << "translators. For more information, please, read: Zinsly, R. \"Region formation\n";
  cout << "techniques for the design of efficient virtual machines\". MsC\n";
  cout << "Thesis. Institute of Computing, 2013 (in portuguese).\n";

  cout << "The tool takes as input a trace of instructions, emulate the formation and \n";
  cout << "execution of  regions, and generates statistics about the region formation\n";
  cout << "techniques. \n";
  cout << "The input trace may be store in multiple files, each one\n";
  cout << "containing a sub-sequence of the trace. Each file is named\n";
  cout << "BASENAME.INDEX.bin.gz, where trace_path is the trace_path of\n";
  cout << "the trace and INDEX indicates the sequence of the trace.\n";
  cout << "The user must provide the trace_path (-b), the start index (-s) and the end \n";
  cout << "index (-e).\n\n";

  cout << "ARGUMENTS:\n";
  clarg::arguments_descriptions(cout, "  ", "\n");
}

int validate_arguments() {
  if (!start_i.was_set()) {
    cerr << "Error: you must provide the start file index."
      << "(use -h for help)\n";
    return 1;
  }

  if (!end_i.was_set()) {
    cerr << "Error: you must provide the end file index."
      << "(use -h for help)\n";
    return 1;
  }

  if (!trace_path.was_set()) {
    cerr << "Error: you must provide the trace_path."
      << "(use -h for help)\n";
    return 1;
  }

  if (!technique.was_set()) {
    cerr << "Error: you must provide the RF Technique: net, lei, mret2, tt or lef"
      << "(use -h for help)\n";
    return 1;
  }

  if (end_i.get_value() < start_i.get_value()) {
    cerr << "Error: start index must be less (<) or equal (=) to end index" 
      << "(use -h for help)\n";
    return 1;
  }

  if (lt.was_set()) {
    if (wt.was_set()) {
      cerr << "Error: both -lt and -lw were set, select only one.\n";
      return 1;
    }
  }

  string chosen_technique = technique.get_value();
  if (chosen_technique == "lei" || chosen_technique == "netplus") {
    if (!bin_path.was_set()) {
      cerr << "Error: you must provide a binary file path with -bin.\n" 
        << "(use -h for help)\n";
      return 1;
    }
  }

  return 0;
}

rf_technique::InstructionSet* load_binary(string binary_path) {
  ud_t ud_obj;
  ud_init(&ud_obj);

  elfio reader;

  reader.load(binary_path);

  Elf_Half sec_num = reader.sections.size();

  ud_set_mode(&ud_obj, 32);
  ud_set_syntax(&ud_obj, UD_SYN_INTEL);

  // Iterate over all sections until find the .text section
  rf_technique::InstructionSet* instructions = new rf_technique::InstructionSet;
  for (int i = 0; i < sec_num; ++i) {
    section* psec = reader.sections[i];
    if (psec->get_name() == ".text" || psec->get_name() == ".init" ||
        psec->get_name() == ".fini" || psec->get_name() == ".plt") {
      // Prepare the .text section to be disassembled
      ud_set_input_buffer(&ud_obj, (unsigned char*) psec->get_data(), psec->get_size());
      ud_set_pc(&ud_obj, psec->get_address());

      while (ud_disassemble(&ud_obj))
        instructions->addInstruction(ud_insn_off(&ud_obj), (char*) ud_insn_ptr(&ud_obj));
    }
  }

  return instructions;
}

rf_technique::RF_Technique* constructRFTechnique(rf_technique::InstructionSet* code_insts, string chosen_technique) {
  rf_technique::RF_Technique* rf;

  unsigned hotness_threshold = rf_threshold.was_set() ? rf_threshold.get_value() : 50;

  if (chosen_technique == "netplus") {
    unsigned limit = 10;
    if (depth_limit.was_set()) 
      limit = depth_limit.get_value();
    rf = new rf_technique::NETPlus(*code_insts, limit, hotness_threshold);
  } else if (chosen_technique == "lei") {
    if (!rf_threshold.was_set())
      hotness_threshold = 35;
    rf = new rf_technique::LEI(*code_insts, hotness_threshold);
  } else if (chosen_technique == "mret2") {
    rf = new rf_technique::MRET2(hotness_threshold);
  } else if (chosen_technique == "tt") {
    rf = new rf_technique::TraceTree(hotness_threshold);
  } else if (chosen_technique == "lef") {
    rf = new rf_technique::LEF(hotness_threshold);
  } else if (chosen_technique == "callspage") {
    rf = new rf_technique::CallsInPage();
  } else {
    rf = new rf_technique::NET(hotness_threshold);
  }

  return rf; 
}

int main(int argc,char** argv) {
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
  trace_io::raw_input_pipe_t in(trace_path.get_value(),
      start_i.get_value(),
      end_i.get_value());

  // Current and next instructions.
  trace_io::trace_item_t current;
  trace_io::trace_item_t next;

  // Fetch the next instruction from the trace
  if (!in.get_next_instruction(current)) {
    cerr << "Error: input trace has no instruction items." << endl;
    return 1;
  }

  unsigned long long sys_threshold;
  if (lt.was_set())
    sys_threshold = LINUX_SYS_THRESHOLD;
  else
    sys_threshold = WINDOWS_SYS_THRESHOLD;

  rf_technique::InstructionSet* code_insts = load_binary(bin_path.get_value());

  string chosen_technique = technique.get_value();

  if (chosen_technique == "lei" || chosen_technique == "netplus") {
    bool DidNotLoadBinary = (code_insts->size() == 0);
    if (DidNotLoadBinary)
      cout << "It was not possible to load the binary file!\nReconstructing it with traces.\n";

/*    trace_io::raw_input_pipe_t in_tmp(trace_path.get_value(), start_i.get_value(), end_i.get_value());
    trace_io::trace_item_t next;

    // We need to load the system instructions for netplus and lei for every execution, because those aren't
    // found in the binary. Futhermore, if it was impossible to load the binary, we should load all instructions to
    // reconstruct it.

    if (!only_user.was_set() || DidNotLoadBinary) {
      while (in_tmp.get_next_instruction(next))
        if (!rf_technique::RF_Technique::is_user_instr(current.addr, sys_threshold)  || DidNotLoadBinary)
          code_insts->addInstruction(next.addr, next.opcode);
    }*/
  }

  rf_technique::RF_Technique* rf = constructRFTechnique(code_insts, chosen_technique);

  if (mix_usr_sys.was_set())
    rf->set_mix_usr_sys(true);
  else
    rf->set_mix_usr_sys(false);

  rf->set_system_threshold(sys_threshold);

  // While there are instructions
  while (in.get_next_instruction(next)) {
    // Process the trace
    if (rf)
      if (!only_user.was_set() || rf->is_user_instr(current.addr))
        rf->process(current.addr, current.opcode, current.length,
            next.addr, next.opcode, next.length);
    current = next;
  }
  if (rf) rf->finish();

  //Print statistics
  if (rf) {
    cout << "Printing OverallStats\n";
    string s("test.dot");
    ofstream overall_stats_f(overall_stats_fname.get_value().c_str());
    rf->rain.printOverallStats(overall_stats_f);
    overall_stats_f.close(); 

    cout << "Printing Regions Dots\n";
    rf->rain.printRegionsDOT(s);

    cout << "Printing RAInStats\n";
    ofstream reg_stats_f(reg_stats_fname.get_value().c_str());
    rf->rain.printRAInStats(reg_stats_f);
    reg_stats_f.close();
  }

  return 0; // Return OK.
}
