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
#include "rf_techniques.h"
#include <iostream>
#include <iomanip>

using namespace rf_technique;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

unsigned long long calls = 0;
unsigned long long calls_in_same_page = 0;
unsigned long long calls_in_touched_page = 0;

#define PAGE_BITS_SIZE 14

unordered_map<unsigned long long, bool> callsTaken;
unordered_map<unsigned long long, bool> touchedPages;

void CallsInPage::process(unsigned long long cur_addr, char cur_opcode[16], char unsigned cur_length, 
    unsigned long long nxt_addr, char nxt_opcode[16], char unsigned nxt_length)
{
  if (!is_system_instr(cur_addr)) return;
  unsigned long long page = last_addr >> PAGE_BITS_SIZE;
  touchedPages[page] = true;

  int opcode  = (int) (unsigned char) last_opcode[0];
  int opcode1 = (int) (unsigned char) last_opcode[1];
  if (opcode == 0xe8 && callsTaken.count(last_addr) == 0) { // was a call
    callsTaken[last_addr] = true;

    calls++;
    if ((cur_addr >> PAGE_BITS_SIZE) == (last_addr >> PAGE_BITS_SIZE))
      calls_in_same_page++;
    if (touchedPages.count(cur_addr >> PAGE_BITS_SIZE) != 0)
      calls_in_touched_page++;
  }

  last_addr = cur_addr;
  for (int i = 0; i < 16; i++)
    last_opcode[i] = cur_opcode[i];
}

void CallsInPage::finish() {
  cout << "Amount of calls: " << calls << "\n";
  cout << "%% of calls in the same page: " << (static_cast<float>(calls_in_same_page)*100)/calls << "\n";
  cout << "%% of calls in a touched page: " << (static_cast<float>(calls_in_touched_page)*100)/calls << "\n";
  exit(1);
}
