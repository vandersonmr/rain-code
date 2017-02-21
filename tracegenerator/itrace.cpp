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

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <utility>
#include <string.h>
#include "pin.H"

static UINT64 BEGINAT = 1E10;
static UINT64 RECORDSIZE = 1E5;
static UINT64 BUFFERSIZE = 100000;

static UINT64 icount = 0;
static UINT64 rcount = 0;
static int hasBegin = 0;
std::stringstream buffer;

/*
 * This is a library to record traces of programs emulated by PIN.
 * The trace is outputed using the raw format and need by converted
 * to be used in RAIn.
 */

UINT32 Usage() {
  cout << "This tool produces an instruction stream w/ runtime metadata" << endl;
  cout << KNOB_BASE::StringKnobSummary();
  cout << endl;
  return 2;
}

VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT c) { icount += c; }

VOID PIN_FAST_ANALYSIS_CALL LogInstruction(ADDRINT address, const char * disasm, unsigned size) {
  ++rcount;
  buffer << "0x" << address << " | "<< disasm << " | " << size << std::endl;

  if (rcount % BUFFERSIZE == 0) {
    printf("%s", buffer.str().c_str());
    buffer.str(std::string());
    fprintf(stderr, "Recorded: %f%%\n", ((double)rcount/RECORDSIZE)*100);
  }

  if (rcount > RECORDSIZE) {
    fprintf(stderr, "end of recording\n");
    exit(0);
  }
}

const char *dumpInstruction(INS ins) {
  ADDRINT address = INS_Address(ins);
  std::stringstream ss;

  for (size_t i = 0; i < INS_Size(ins); i++)
    ss << "0x" << setfill('0') << setw(2) << hex << (((unsigned int) *(unsigned char*)(address + i)) & 0xFF) << " ";

  return strdup(ss.str().c_str());
}

VOID TraceInstructions(INS ins, VOID *arg) {
  const char * disasm = dumpInstruction(ins);

  INS_InsertCall( ins,
      IPOINT_BEFORE,
      (AFUNPTR) LogInstruction,
      IARG_INST_PTR, // address of instruction
      IARG_PTR, disasm, // disassembled string
      IARG_UINT32, INS_Size(ins),
      IARG_END);
}

VOID Trace(TRACE trace, VOID *v) {
  if (!hasBegin) {
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) 
      BBL_InsertCall(bbl, IPOINT_ANYWHERE, AFUNPTR(docount), IARG_FAST_ANALYSIS_CALL,
          IARG_UINT32, BBL_NumIns(bbl), IARG_END);

    if (icount > BEGINAT) {
      fprintf(stderr, "Starting recording\n");
      INS_AddInstrumentFunction(TraceInstructions, 0);
      hasBegin = 1;
    }
  }
}

int main(int argc, char **argv) {
  PIN_SetSyntaxXED();
  PIN_InitSymbols();

  if (PIN_Init(argc, argv)) return Usage();
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0)
      BEGINAT = strtol(argv[i+1], &argv[i+2]-1, 10);

    else if (strcmp(argv[i], "-s") == 0)
      RECORDSIZE = strtol(argv[i+1], &argv[i+2]-1, 10);

    else if (strcmp(argv[i], "-bs") == 0)
      BUFFERSIZE = strtol(argv[i+1], &argv[i+2]-1, 10);
  }

  fprintf(stderr, "Set starting recording when %lu, ending when %lu and buffer size as %lu\n", 
      BEGINAT, RECORDSIZE, BUFFERSIZE);

  TRACE_AddInstrumentFunction(Trace, 0);

  PIN_StartProgram();

  return 0;
}
