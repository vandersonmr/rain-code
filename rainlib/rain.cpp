/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   Edson Borin (edson@ic.unicamp.br), and                                *
 *   Vanderson Rosario (vandersonmr2@gmail.com)                            *
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
#include "rain.h"

#include <iostream>
#include <fstream>
#include <sstream>  // stringstream
#include <iomanip>  // setbase
#include <stdlib.h> // exit
#include <vector>   // vector
#include <algorithm>// sort
#include <assert.h>

using namespace std;
using namespace rain;

#define DEBUG

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

Region::Node::Node() : region(NULL), freq_counter(0), 
  out_edges(NULL), in_edges(NULL) {}

Region::Node::Node(unsigned long long a) : region(NULL), freq_counter(0), 
  out_edges(NULL), in_edges(NULL),
  addr(a) {}

Region::Node::~Node() {
  EdgeListItem* it = out_edges;
  while (it) {
    EdgeListItem* next = it->next;
    delete it;
    it = next;
  }

  it = in_edges;
  while (it) {
    EdgeListItem* next = it->next;
    delete it;
    it = next;
  }
}

Region::Edge* Region::Node::findInEdge(unsigned long long prev_ip) {
  if (!in_edges) return NULL;
  if (in_edges->edge->src->getAddress() == prev_ip) return in_edges->edge;

  EdgeListItem* prev = NULL;
  for (EdgeListItem* it = in_edges; it; it = it->next) {
    if (it->edge->src->getAddress() == prev_ip) {

      if (prev != NULL) {
        // Move item to beginning of list...
        prev->next = it->next;
        it->next = in_edges;
        in_edges = it;
      }

      return it->edge;
    }
    prev = it;
  }
  return NULL;
}

Region::Edge* Region::Node::findOutEdge(unsigned long long next_ip) {
  if (!out_edges) return NULL;
  if (!(out_edges->edge)) return NULL;
  if (!(out_edges->edge->tgt)) return NULL;
  if (out_edges->edge->tgt->getAddress() == next_ip) return out_edges->edge;

  EdgeListItem* prev = NULL;
  for (EdgeListItem* it = out_edges; it; it = it->next) {
    if (it->edge->tgt->getAddress() == next_ip) {

      if (prev != NULL) {
        // Move item to beginning of list...
        prev->next = it->next;
        it->next = out_edges;
        out_edges = it;
      }

      return it->edge;
    }
    prev = it;
  }
  return NULL;
}

Region::Edge* Region::Node::findOutEdge(Region::Node* target) const {
  for (EdgeListItem* it = out_edges; it; it = it->next)
    if (it->edge->tgt == target)
      return it->edge;
  return NULL;
}

Region::Edge* Region::Node::findInEdge(Region::Node* source) const {
  for (EdgeListItem* it = in_edges; it; it = it->next)
    if (it->edge->src == source)
      return it->edge;
  return NULL;
}

void Region::Node::insertOutEdge(Region::Edge* ed, Region::Node* target) {
#ifdef DEBUG
  assert(ed->src == this);
  assert(ed->tgt == target);
  // Check for edge duplication
  assert(findOutEdge(target) == NULL);
#endif 

  Region::EdgeListItem* it = new Region::EdgeListItem();
  it->edge = ed;
  it->next = out_edges;
  out_edges = it;
}

void Region::Node::insertInEdge(Region::Edge* ed, Region::Node* source) {
#ifdef DEBUG
  assert(ed->tgt == this);
  assert(ed->src == source);
  // Check for edge duplication
  //assert(findInEdge(source) == NULL);
#endif 

  Region::EdgeListItem* it = new Region::EdgeListItem();
  it->edge = ed;
  it->next = in_edges;
  in_edges = it;
}

Region::~Region() {
  // Remove all edges.
  for (auto e : region_inner_edges)
    delete e;

  region_inner_edges.clear();

  // Remove all nodes.
  for(auto node : nodes)
    delete node;

  nodes.clear();

  // remove pointer to entry and exit nodes
  entry_nodes.clear();
  exit_nodes.clear();
}

void Region::moveAndDestroy(Region* reg, unordered_map<unsigned long long, Node*>& ren) {
  unordered_map<Node*, Node*> translation_table;
  for (auto node : reg->nodes) {
      node->region = this;
      // If aready there a node if this address
      if (getNode(node->getAddress()) != nullptr)
        translation_table[node] = getNode(node->getAddress());
      else
        nodes.insert(node);
  }

  // Translate entries
  for (auto node : reg->entry_nodes) {
    if (translation_table.count(node) == 0) {
      entry_nodes.insert(node);
    } else {
      entry_nodes.insert(translation_table[node]);
      ren[node->getAddress()] = translation_table[node];
    }
  }

  // Translate exits
  for (auto node : reg->exit_nodes) {
    if (translation_table.count(node) == 0)
      exit_nodes.insert(node);
    else
      exit_nodes.insert(translation_table[node]);
  }

  for (auto pair : translation_table) {
    Node* src = pair.first;
    Node* tgt = pair.second;
    tgt->freq_counter += src->freq_counter;

    for (EdgeListItem* prev = src->out_edges; prev != NULL; prev = prev->next) {
      Region::Edge* ed = prev->edge;
      if (translation_table.count(ed->src) != 0) ed->src = translation_table[ed->src];
      if (translation_table.count(ed->tgt) != 0) ed->tgt = translation_table[ed->tgt];

      Region::Edge* tgt_ed = tgt->findOutEdge(ed->tgt);
      if (tgt_ed)
        tgt_ed->freq_counter += ed->freq_counter;
      else
        tgt->insertOutEdge(ed, ed->tgt);
    }

    for (EdgeListItem* prev = src->in_edges; prev != NULL; prev = prev->next) {
      Region::Edge* ed = prev->edge;
      if (translation_table.count(ed->src) != 0) ed->src = translation_table[ed->src];
      if (translation_table.count(ed->tgt) != 0) ed->tgt = translation_table[ed->tgt];

      Region::Edge* tgt_ed = tgt->findInEdge(ed->src);
      if (tgt_ed)
        tgt_ed->freq_counter += ed->freq_counter;
      else
        tgt->insertInEdge(ed, ed->src);
    }
    delete src;
  }

  reg->alive = false;
  reg->region_inner_edges.clear();
  reg->nodes.clear();
  reg->entry_nodes.clear();
  reg->exit_nodes.clear();
  //delete reg;
}

unsigned Region::getNumberOfSideEntries() {
  unsigned amount = 0;
  for (auto node : entry_nodes) {
      auto it = node->in_edges;
      while (it != NULL) {
        if (node->region->isInnerEdge(it->edge)) amount++;
        it = it->next;
      }
  }
  return amount;
}

Region::Node* Region::getNode(unsigned long long addr) {
  for (auto n : nodes)
    if (n->getAddress() == addr) return n;
  return nullptr;
}

void Region::insertRegOutEdge(Edge* ed) {
#ifdef DEBUG
  assert(ed->src->region == this);
  // assert(ed->tgt->region != this); // This assert may not be valid. A side
  // exit may reach the entry of the same region. It is still a valid inter
  // region edge.
#endif 

  EdgeListItem* it = new EdgeListItem();
  it->edge = ed;
  it->next = reg_out_edges;
  reg_out_edges = it;
}

void Region::insertRegInEdge(Region::Edge* ed) {
#ifdef DEBUG
  // assert(ed->src->region != this); // This assert may not be valid. A side
  // exit may reach the entry of the same region. It is still a valid inter
  // region edge.
  assert(ed->tgt->region == this);
#endif 

  EdgeListItem* it = new EdgeListItem();
  it->edge = ed;
  it->next = reg_in_edges;
  reg_in_edges = it;
}

Region::Edge* Region::createInnerRegionEdge(Region::Node* src, Region::Node* tgt) {
  Edge* ed = new Edge(src,tgt);
  src->insertOutEdge(ed,tgt);
  tgt->insertInEdge(ed,src);
  region_inner_edges.insert(ed);
  return ed;
}


Region::Edge* RAIn::queryNext(unsigned long long next_ip) {
  if (cur_node == nte) {
    // NTE node (treated separatedely for efficiency reasons)
    map<unsigned long long, Region::Edge*>::iterator it =
      nte_out_edges_map.find(next_ip);
    if (it != nte_out_edges_map.end())
      return it->second; // Return existing NTE out edge
    else {
      // Search for region entries, if there is none, return nte_loop_edge
      if (region_entry_nodes.find(next_ip) != region_entry_nodes.end()) {
        // edge representing transition from nte to region missing.
        return NULL;
      }
      else {
        // transition from nte to nte
        return nte_loop_edge;
      }
    }
  }
  else {
    // Region node
    Region::Edge* edge = cur_node->findOutEdge(next_ip);
    return edge;
  }
}

Region::Edge* RAIn::addNext(unsigned long long next_ip) {
  // Sanity checking
  DBG_ASSERT(queryNext(next_ip) == NULL);
  Region::Edge* edg = NULL;
  Region::Node* next_node = NULL;

  // Search for region entries.
  unordered_map<unsigned long long, Region::Node*>::iterator it = 
    region_entry_nodes.find(next_ip);

  if(it != region_entry_nodes.end())
    next_node = it->second;
  else
    next_node = NULL;

  if (cur_node == nte) {
    if (next_node == NULL) {
      cerr << "Error, addNext should be not called when "
        << "queryNext returns a valid edge." << "\n";
      exit (1);
    } else {
      // Add edge from nte to region entry.
      edg = createInterRegionEdge(nte, next_node);
    }
  } else {
    // current node belongs to region.
    if (next_node == NULL) {
      Region* cur_reg = cur_node->region;
      next_node = cur_reg->getNode(next_ip);

      if (next_node == NULL) {
        // add edge from cur_node to nte (back to emulation manager)
        edg = cur_node->findOutEdge(nte);
        if (!edg) edg = createInterRegionEdge(cur_node, nte);
      } else {
        edg = cur_reg->createInnerRegionEdge(cur_node, next_node);
      }
    } else {
      // add edge from cur_node to region_entry (link regions)
      edg = createInterRegionEdge(cur_node, next_node);
    }
  }

  DBG_ASSERT(edg != NULL);
  return edg;
}

void RAIn::executeEdge(Region::Edge* edge) {
  if (edge->src != cur_node) cur_node = edge->src;

  cur_node = edge->tgt;
  edge->freq_counter++;
  cur_node->freq_counter++;
  executed_freq++;

  if (cur_node->region != 0) { 
    if (cur_node->region->isFromExpansion)
      executed_expasion_freq++;
  }

  if (edge->src->region != 0 && edge->tgt->region != 0) {
    Region* src_reg = edge->src->region;
    Region* tgt_reg = edge->tgt->region;

    if (src_reg != tgt_reg)
      region_transitions++;
  }
}

Region* RAIn::createRegion() {
  Region* region;
  region = new Region();
  region->id = region_id_generator++;
  regions[region->id] = region;
  region_start_freq[region->id] = executed_freq;
  return region;
}

void RAIn::setEntry(Region::Node* node) { 
  region_entry_nodes[node->getAddress()] = node;
  node->region->setEntryNode(node);
}

void RAIn::setExit(Region::Node* node) {
  node->region->setExitNode(node);
}

Region::Edge* RAIn::createInterRegionEdge(Region::Node* src, Region::Node* tgt) {
  Region::Edge* ed = new Region::Edge(src,tgt);
  src->insertOutEdge(ed,tgt);
  tgt->insertInEdge(ed,src);

  if (src->region)
    src->region->insertRegOutEdge(ed);
  if (tgt->region)
    tgt->region->insertRegInEdge(ed);

  inter_region_edges.push_back(ed);
  if (src == nte)
    nte_out_edges_map[tgt->getAddress()] = ed;
  return ed;
}

void RAIn::printRAInStats(ostream& stats_f) {
  // Print statistics for regions
  printRegionsStats(stats_f);
  // Print statistics for NTE
  stats_f << "0" << "," << nte->freq_counter << "\n";
}

unsigned long long Region::allNodesFreq() const {
  unsigned long long c = 0;
  for (Region::Node* node : nodes)
    c += node->freq_counter;

  return c;
}

unsigned long long Region::entryNodesFreq() const {
  unsigned long long c = 0;
  for (auto entry_node : entry_nodes) {
    for (EdgeListItem* it = entry_node->in_edges; it; it=it->next) {
      Edge* e = it->edge;
      c += e->freq_counter;
    }
  }
  return c;
}

unsigned long long Region::exitNodesFreq() const {
  unsigned long long c = 0;
  for (auto exit_node : exit_nodes)
    c += exit_node->freq_counter;
  return c;
}

bool Region::isInnerEdge(Region::Edge* e) const {
  for (auto it=region_inner_edges.begin(); it != region_inner_edges.end(); it++) {
    if ( (*it) == e)
      return true;
  }
  return false;
}

unsigned long long Region::mainExitsFreq() const {
  unsigned long long c = 0;
  for (auto exit_node : exit_nodes) {
    for (EdgeListItem* eit = exit_node->out_edges; eit; eit=eit->next) {
      Edge* e = eit->edge;
      // Is it an exit edge?
      if (!isInnerEdge(e))
        c += e->freq_counter;
    }
  }
  return c;
}

unsigned long long Region::externalEntriesFreq() const {
  unsigned long long c = 0;
  for (auto entry_node : entry_nodes) {
    for (EdgeListItem* it = entry_node->in_edges; it; it=it->next) {
      Edge* e = it->edge;
      if (e->src->region != e->tgt->region)
        c += e->freq_counter;
    }
  }
  return c;
}

bool Region::isSpannedCycle() const {
  for (auto entry_node : entry_nodes) {
    for (EdgeListItem* eit = entry_node->in_edges; eit; eit=eit->next) {
      Edge* e = eit->edge;
      if (e->src->region == e->tgt->region)
        return true;
    }
  }
  return false;
}

void RAIn::printRegionsStats(ostream& stats_f) {
  stats_f << "Region," 
    << "# Nodes," 
    << "All Nodes Freq," 
    << "Entry Nodes Freq," 
    << "Exit Nodes Freq," 
    << "External Entries,"
    << "\n";

  for (auto i : regions) {
    Region* r = i.second;

    stats_f << r->id << ","
      << r->nodes.size() << ","
      << r->allNodesFreq() << ","
      << r->entryNodesFreq() << ","
      << r->exitNodesFreq() << ","
      << r->externalEntriesFreq() << ","
      << "\n";
  }
}

struct cov_less_than_key
{
  inline bool operator() (const pair<Region*,unsigned long long>& p1,
      const pair<Region*,unsigned long long>& p2) {
    return (p1.second >= p2.second);
  }
};

void RAIn::printOverallStats(ostream& stats_f) {
  unsigned long long total_reg = 0;
  unsigned long long total_stat_reg_size = 0;
  unsigned long long total_reg_entries = 0;
  unsigned long long total_reg_external_entries = 0;
  unsigned long long total_reg_main_exits = 0;
  unsigned long long total_reg_freq = 0;
  unsigned long long total_side_entries = 0;
  unsigned long long nte_freq = nte->freq_counter;
  unsigned long long total_reg_oficial_exit = 0;
  unsigned long long total_spanned_cycles = 0;
  unordered_map<unsigned long long,unsigned> unique_instrs;
  unsigned long long _70_cover_set_regs = 0;
  unsigned long long _80_cover_set_regs = 0;
  unsigned long long _90_cover_set_regs = 0;
  unsigned long long _70_cover_set_instrs = 0;
  unsigned long long _80_cover_set_instrs = 0;
  unsigned long long _90_cover_set_instrs = 0;

  vector< pair<Region*,unsigned long long> > region_cov;
  for (auto rit : regions) {
    Region* r = rit.second;

    if (!r->alive) continue;
    total_side_entries += r->getNumberOfSideEntries();

    assert(r != nullptr && "Region is null in printOverallStats");
    total_reg += 1;
    total_stat_reg_size += r->nodes.size();
    total_reg_entries += r->entryNodesFreq();
    total_reg_external_entries += r->externalEntriesFreq();
    total_reg_main_exits += r->mainExitsFreq();

    if (r->isSpannedCycle()) total_spanned_cycles++;

    unsigned long long allNodesFreq = r->allNodesFreq();
    total_reg_freq += allNodesFreq;
    if (allNodesFreq > 0)
      region_cov.push_back(pair<Region*, unsigned long long>(r, allNodesFreq));

    for (Region::Node* node : r->nodes)
      unique_instrs[node->getAddress()] = 1;
  }

  unsigned long long total_unique_instrs = unique_instrs.size();

  // Sort regions by coverage (# of instructions executed) or by avg_dyn_size?
  // 90% cover set => sort by coverage (r->allNodesFreq())
  std::sort(region_cov.begin(), region_cov.end(), cov_less_than_key());
  vector< pair<Region*,unsigned long long> >::const_iterator rcit;
  unsigned long long acc = 0;
  unsigned long long cov_num_regs = 0;
  unsigned long long cov_num_inst = 0;
  for (rcit=region_cov.begin(); rcit != region_cov.end(); rcit++) {
    acc += rcit->second;
    double coverage = (double) acc / (double) (total_reg_freq+nte_freq);

    assert(total_reg_freq+nte_freq != 0 && "Total_reg_freq is 0 and is dividing");

    cov_num_inst += rcit->first->nodes.size();
    cov_num_regs++;

    if (coverage > 0.7 && _70_cover_set_regs == 0) {
      _70_cover_set_instrs = cov_num_inst;
      _70_cover_set_regs   = cov_num_regs;
    }

    if (coverage > 0.8 && _80_cover_set_regs == 0) {
      _80_cover_set_instrs = cov_num_inst;
      _80_cover_set_regs   = cov_num_regs;
    }

    if (coverage > 0.9) {
      _90_cover_set_instrs = cov_num_inst;
      _90_cover_set_regs = cov_num_regs;
      break;
    }
  }

  stats_f << "reg_dyn_inst_count" << "," << total_reg_freq << ",Freq. of instructions emulated by regions" << "\n";
  stats_f << "reg_stat_instr_count" << "," << total_stat_reg_size  << ",Static # of instructions translated into regions" << "\n";
  stats_f << "reg_uniq_instr_count" << "," << total_unique_instrs << ",# of unique instructions included on regions" << "\n";
  stats_f << "reg_dyn_entries" <<  "," << total_reg_entries << ",Regions entry frequency count" << "\n";
  stats_f << "number_of_regions" <<  "," << total_reg << ",Total number of regions" << "\n";
  stats_f << "interp_dyn_inst_count" << "," << nte_freq << ",Freq. of instruction emulated by interpretation" << "\n";

  assert(total_reg_entries != 0 && "total_reg_entries is 0 and it's dividing");
  stats_f << "avg_dyn_reg_size" << "," <<
    (double) total_reg_freq / (double) total_reg_entries
    << "," << "Average dynamic region size." << "\n";

  assert(total_reg != 0 && "region.size() is 0 and it's dividing");
  stats_f << "avg_stat_reg_size" << "," <<
    (double) total_stat_reg_size / (double) total_reg
    << "," << "Average static region size." << "\n";

  stats_f << "dyn_reg_coverage" << "," << 
    (double) total_reg_freq / (double) (total_reg_freq + nte_freq)
    << "," << "Dynamic region coverage." << "\n";

  assert(total_unique_instrs != 0 && "total unique is 0 and it's dividing");
  stats_f << "code_duplication" << "," << 
    (double) total_stat_reg_size / (double) total_unique_instrs
    << "," << "Region code duplication" << "\n";
  stats_f << "completion_ratio" << "," << 
    (double) total_reg_main_exits / (double) total_reg_entries
    << "," << "Completion Ratio" << "\n";

  stats_f << "num_expasions" << "," << expansions << "," << "Number of Expansions" << "\n";
  stats_f << "side_entries" << "," << total_side_entries/ (double) total_reg << "," << "Side Entries" << "\n";
  stats_f << "region_transitions" << "," << region_transitions << ","
    << "Number of regions transitions" << "\n";
  stats_f << "num_counters" << "," << number_of_counters << ","
    << "Number of used counters" << "\n";
  stats_f << "spanned_cycles" << "," << total_spanned_cycles / (double) total_reg << ","
    << "Spanned Cycle Ratio" << "\n";
  stats_f << "spanned_exec_ration" <<  "," << 1-(total_reg_external_entries / (double) total_reg_entries) <<
    ",Spanned execution ratio" << "\n";

  stats_f << "70_cover_set_regs" << "," << _70_cover_set_regs
    << "," << "minumun number of regions to cover 70% of dynamic execution" << "\n";
  stats_f << "80_cover_set_regs" << "," << _80_cover_set_regs
    << "," << "minumun number of regions to cover 80% of dynamic execution" << "\n";
  stats_f << "90_cover_set_regs" << "," << _90_cover_set_regs
    << "," << "minumun number of regions to cover 90% of dynamic execution" << "\n";
  stats_f << "70_cover_set_instrs" << "," << _70_cover_set_instrs
    << "," << "minumun number of static instructions on regions to cover 70% of dynamic execution" << "\n";
  stats_f << "80_cover_set_instrs" << "," << _80_cover_set_instrs
    << "," << "minumun number of static instructions on regions to cover 80% of dynamic execution" << "\n";
  stats_f << "90_cover_set_instrs" << "," << _90_cover_set_instrs
    << "," << "minumun number of static instructions on regions to cover 90% of dynamic execution" << "\n";

  stats_f << "executed_expasion_freq" << "," << executed_expasion_freq << "," << "Total Exec. Freq. From Expanded Regs.\n";
}

void RAIn::printRegionDOT(Region* region, ostream& reg) {
  reg << "digraph G{" << "\n";

  map<Region::Node*,unsigned> node_id;
  unsigned id_gen = 1;
  // For each node.
  reg << "/* nodes */" << "\n";
  reg << "/* Start Freq.: " << region_start_freq[region->id] << " */" << "\n";
  reg << "/* entry: ";
  for (Region::Node* n : region->entry_nodes) {
    reg << "0x" << std::hex << n->getAddress() << std::dec << " ";
  }
  reg << " */" << "\n";

  for (Region::Node* n : region->nodes) {
    node_id[n] = id_gen++;
    reg << "  n" << node_id[n] << " [label=\"0x" << std::hex << n->getAddress() << "\"]" << "\n";
  }

  reg << "/* edges */" << "\n";
  for (Region::Node* n : region->nodes) {
    // For each out edge.
    for (Region::EdgeListItem* i = n->out_edges; i; i=i->next) {
      Region::Edge* edg = i->edge;
      reg << "n" << node_id[edg->src] << " -> " << "n" << node_id[edg->tgt] << ";" << "\n";
    }
    // For each in edge.
    for (Region::EdgeListItem* i = n->in_edges; i; i=i->next) {
      Region::Edge* edg = i->edge;
      reg << "n" << node_id[edg->src] << " -> " << "n" << node_id[edg->tgt] << ";" << "\n";
    }
  }

  reg << "}" << "\n";
}

void RAIn::printRegionsDOT(string& dotf_prefix) {
  map<unsigned, Region*>::iterator it,end;
  it = regions.begin();
  end = regions.end();
  for(unsigned c=1; it != end; it++, c++) {
    ostringstream fn;
    fn << dotf_prefix << std::setfill ('0') << std::setw (4) << c << ".dot"; 
    ofstream dotf(fn.str().c_str());
    printRegionDOT(it->second, dotf);
    dotf.close();
  }
}
