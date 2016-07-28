/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   Edson Borin (edson@ic.unicamp.br), and                                *
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

using namespace std;
using namespace rain;

#ifdef DEBUG
#include <assert.h>
#define DBG_ASSERT(cond) assert(cond)
#else
#define DBG_ASSERT(cond)
#endif

Region::Node::Node() : region(NULL), freq_counter(0), 
  out_edges(NULL), in_edges(NULL) 
{}

Region::Node::Node(unsigned long long a) : region(NULL), freq_counter(0), 
  out_edges(NULL), in_edges(NULL),
  addr(a)
{}

Region::Node::~Node()
{
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

Region::Edge* Region::Node::findOutEdge(unsigned long long next_ip)
{
  if (!out_edges) return NULL;
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

Region::Edge* Region::Node::findOutEdge(Region::Node* target) const
{
  for (EdgeListItem* it = out_edges; it; it = it->next) {
    if (it->edge->tgt == target) {
      return it->edge;
    }
  }
  return NULL;
}

Region::Edge* Region::Node::findInEdge(Region::Node* source) const
{
  for (EdgeListItem* it = in_edges; it; it = it->next) {
    if (it->edge->src == source) {
      return it->edge;
    }
  }
  return NULL;
}

void Region::Node::insertOutEdge(Region::Edge* ed, Region::Node* target)
{
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

void Region::Node::insertInEdge(Region::Edge* ed, Region::Node* source)
{
#ifdef DEBUG
  assert(ed->tgt == this);
  assert(ed->src == source);
  // Check for edge duplication
  assert(findInEdge(source) == NULL);
#endif 

  Region::EdgeListItem* it = new Region::EdgeListItem();
  it->edge = ed;
  it->next = in_edges;
  in_edges = it;
}

Region::~Region()
{
  // Remove all edges.
  list<Edge*>::iterator eit = region_inner_edges.begin();
  for (; eit!= region_inner_edges.end(); eit++) {
    Edge* e = *eit;
    delete e;
  }
  region_inner_edges.clear();

  // Remove all nodes.
  for(list<Node*>::iterator nit = nodes.begin(); nit != nodes.end(); nit++) {
    Region::Node* node = (*nit);
    delete node;
  }
  nodes.clear();

  // Remove pointer to entry and exit nodes
  entry_nodes.clear();
  exit_nodes.clear();
}

void Region::insertRegOutEdge(Edge* ed)
{
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

void Region::insertRegInEdge(Region::Edge* ed)
{
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

Region::Edge* Region::createInnerRegionEdge(Region::Node* src, Region::Node* tgt)
{
  Edge* ed = new Edge(src,tgt);
  src->insertOutEdge(ed,tgt);
  tgt->insertInEdge(ed,src);
  region_inner_edges.push_back(ed);
  return ed;
}

Region::Edge* RAIn::queryNext(unsigned long long next_ip)
{
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
    return cur_node->findOutEdge(next_ip);
  }
}

Region::Edge* RAIn::addNext(unsigned long long next_ip)
{
  // Sanity checking
  DBG_ASSERT(queryNext(next_ip) == NULL);
  Region::Edge* edg = NULL;
  Region::Node* next_node = NULL;

  // Search for region entries.
  map<unsigned long long, Region::Node*>::iterator it = 
    region_entry_nodes.find(next_ip);
  if(it != region_entry_nodes.end())
    next_node = it->second;
  else
    next_node = NULL;

  if (cur_node == nte) {
    if (next_node == NULL) {
      cerr << "Error, addNext should be not called when "
        << "queryNext returns a valid edge." << endl;
      exit (1);
    }
    else {
      // Add edge from nte to region entry.
      edg = createInterRegionEdge(nte, next_node);
    }
  }
  else {
    // current node belongs to region.
    if (next_node == NULL) {
      // add edge from cur_node to nte (back to emulation manager) 
      edg = cur_node->findOutEdge(nte);
      if (!edg) edg = createInterRegionEdge(cur_node, nte);
    }
    else {
      // add edge from cur_node to region_entry (link regions)
      edg = createInterRegionEdge(cur_node, next_node);
    }
  }

  DBG_ASSERT(edg != NULL);
  return edg;
}

void RAIn::executeEdge(Region::Edge* edge)
{
  DBG_ASSERT(edge->src == cur_node);
  cur_node = edge->tgt;
  edge->freq_counter++;
  cur_node->freq_counter++;
}

Region* RAIn::createRegion()
{
  Region* region; 
  region = new Region();
  region->id = region_id_generator++;
  regions[region->id] = region;
  return region;
}

void RAIn::setEntry(Region::Node* node)
{ 
  region_entry_nodes[node->getAddress()] = node;
  node->region->setEntryNode(node);
}

void RAIn::setExit(Region::Node* node)
{
  node->region->setExitNode(node);
}

Region::Edge* RAIn::createInterRegionEdge(Region::Node* src, Region::Node* tgt)
{
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

void RAIn::printRAInStats(ostream& stats_f)
{
  // Print statistics for regions
  printRegionsStats(stats_f);
  // Print statistics for NTE
  stats_f << "0" << "," << nte->freq_counter << endl;
}

unsigned long long Region::allNodesFreq() const
{
  unsigned long long c = 0;
  for (list<Node*>::const_iterator it = nodes.begin(); it!=nodes.end(); it++) {
    c += (*it)->freq_counter;
  }
  std::cout << std::endl;
  return c;
}

unsigned long long Region::entryNodesFreq() const
{
  unsigned long long c = 0;
  for (list<Node*>::const_iterator it = entry_nodes.begin(); it!=entry_nodes.end(); it++) {
    c += (*it)->freq_counter;
  }
  return c;
}

unsigned long long Region::exitNodesFreq() const
{
  unsigned long long c = 0;
  for (list<Node*>::const_iterator it = exit_nodes.begin(); it!=exit_nodes.end(); it++) {
    c += (*it)->freq_counter;
  }
  return c;
}

bool Region::isInnerEdge(Region::Edge* e) const
{
  list<Edge*>::const_iterator it;
  for (it=region_inner_edges.begin(); it != region_inner_edges.end(); it++) {
    if ( (*it) == e)
      return true;
  }
  return false;
}

unsigned long long Region::mainExitsFreq() const
{
  unsigned long long c = 0;
  list<Node*>::const_iterator it;

  for (it=exit_nodes.begin(); it != exit_nodes.end(); it++) {

    for (EdgeListItem* eit = (*it)->out_edges; eit; eit=eit->next) {
      Edge* e = eit->edge;
      // Is it an exit edge? 
      if (!isInnerEdge(e)) {
        c += e->freq_counter;
      }
    }

  }
  return c;
}

unsigned long long Region::externalEntriesFreq() const
{
  unsigned long long c = 0;
  for (EdgeListItem* it = reg_in_edges; it; it=it->next) {
    Edge* e = it->edge;
    c += e->freq_counter;
  }
  return c;
}

void RAIn::printRegionsStats(ostream& stats_f)
{
  map<unsigned, Region*>::iterator rit;

  stats_f << "Region," 
    << "# Nodes," 
    << "All Nodes Freq," 
    << "Entry Nodes Freq," 
    << "Exit Nodes Freq," 
    << "External Entries,"
    << endl;
  for (rit = regions.begin(); rit != regions.end(); rit++)
  {
    Region* r = rit->second;

    stats_f << r->id << ","
      << r->nodes.size() << "," 
      << r->allNodesFreq() << "," 
      << r->entryNodesFreq() << "," 
      << r->exitNodesFreq() << "," 
      << r->externalEntriesFreq() << ","
      << endl;
  }
}

struct cov_less_than_key
{
  inline bool operator() (const pair<Region*,unsigned long long>& p1, 
      const pair<Region*,unsigned long long>& p2)
  {
    return (p1.second >= p2.second);
  }
};

void RAIn::printOverallStats(ostream& stats_f)
{
  unsigned long long total_stat_reg_size = 0;
  unsigned long long total_reg_entries = 0;
  unsigned long long total_reg_main_exits = 0;
  unsigned long long total_reg_freq = 0;
  unsigned long long nte_freq = nte->freq_counter;
  unsigned long long total_reg_oficial_exit = 0;
  map<unsigned long long,unsigned> unique_instrs;
  unsigned long long _90_cover_set_regs = 0;
  unsigned long long _90_cover_set_instrs = 0;

  vector< pair<Region*,unsigned long long> > region_cov;
  map<unsigned, Region*>::iterator rit;
  for (rit = regions.begin(); rit != regions.end(); rit++)
  {
    Region* r = rit->second;
    total_stat_reg_size += r->nodes.size();
    total_reg_entries += r->externalEntriesFreq();
    total_reg_main_exits += r->mainExitsFreq();
    total_reg_freq += r->allNodesFreq();
    region_cov.push_back(pair<Region*,unsigned long long>(r,total_reg_freq));
    list<Region::Node*>::const_iterator it;
    for (it=r->nodes.begin(); it!=r->nodes.end(); it++) {
      unique_instrs[(*it)->getAddress()] = 1;
    }
  }
  unsigned long long total_unique_instrs = unique_instrs.size();

  // Sort regions by coverage (# of instructions executed) or by avg_dyn_size?
  // 90% cover set => sort by coverage (r->allNodesFreq())
  std::sort(region_cov.begin(), region_cov.end(), cov_less_than_key());
  vector< pair<Region*,unsigned long long> >::const_iterator rcit;
  unsigned long long acc = 0; 
  for (rcit=region_cov.begin(); rcit != region_cov.end(); rcit++) {
    acc += rcit->second;
    _90_cover_set_regs ++;
    _90_cover_set_instrs += rcit->first->nodes.size();
    double coverage = (double) acc / (double) (total_reg_freq+nte_freq) ;
    if (coverage > 0.9) {
      break;
    }
  }

  stats_f << "reg_dyn_inst_count" << "," << total_reg_freq << ",Freq. of instructions emulated by regions" << endl;
  stats_f << "reg_stat_instr_count" << "," << total_stat_reg_size  << ",Static # of instructions translated into regions" << endl;
  stats_f << "reg_uniq_instr_count" << "," << total_unique_instrs << ",# of unique instructions included on regions" << endl;
  stats_f << "reg_dyn_entries" <<  "," << total_reg_entries << ",Regions entry frequency count" << endl;
  stats_f << "number_of_regions" <<  "," << regions.size() << ",Total number of regions" << endl;
  stats_f << "interp_dyn_inst_count" << "," << nte_freq << ",Freq. of instruction emulated by interpretation" << endl;
  stats_f << "avg_dyn_reg_size" << "," << 
    (double) total_reg_freq / (double) total_reg_entries 
    << "," << "Average dynamic region size." << endl;
  stats_f << "avg_stat_reg_size" << "," << 
    (double) total_stat_reg_size / (double) regions.size() 
    << "," << "Average static region size." << endl;
  stats_f << "dyn_reg_coverage" << "," << 
    (double) total_reg_freq / (double) (total_reg_freq + nte_freq)
    << "," << "Dynamic region coverage." << endl;
  //stats_f << "stat_reg_coverage" << "," << 
  //  (double) total_reg_freq / (double) (total_reg_freq + total_stat_instr_count)
  //	  << "Static region coverage: reg_stat_instr_count / (reg_stat_instr_count+total_stat_instr_count)" << endl;
  stats_f << "code_duplication" << "," << 
    (double) total_stat_reg_size / (double) total_unique_instrs
    << "," << "Region code duplication" << endl;
  stats_f << "completion_ratio" << "," << 
    (double) total_reg_main_exits / (double) total_reg_entries
    << "," << "Completion Ratio" << endl;
  stats_f << "90_cover_set_regs" << "," << _90_cover_set_regs 
    << "," << "minumun number of regions to cover 90% of dynamic execution" << endl;
  stats_f << "90_cover_set_instrs" << "," << _90_cover_set_instrs 
    << "," << "minumun number of static instructions on regions to cover 90% of dynamic execution" << endl;

  // Total number of regions (number_of_regions) = regions.size()
  // 90% cover-set (90_cover_set) = 
  // completion ratio (completion_ratio) = total_reg_main_exits / total_reg_entries 
  // code duplication (code_duplication)
  // avg static size  (av_stat_reg_size)
  // avg dynamic size (avg_dyn_reg_size)
}

void RAIn::printRegionDOT(Region* region, ostream& reg)
{
  reg << "digraph G{" << endl;

  map<Region::Node*,unsigned> node_id;
  unsigned id_gen = 1;
  // For each node.
  reg << "/* nodes */" << endl;
  for (list<Region::Node*>::const_iterator nit = region->nodes.begin(); 
      nit != region->nodes.end(); nit++) {
    Region::Node* n = *nit;
    node_id[n] = id_gen++;
    reg << "  n" << node_id[n] << " [label=\"0x" << n->getAddress() << "\"]" << endl;
  }

  reg << "/* edges */" << endl;
  for (list<Region::Node*>::const_iterator nit = region->nodes.begin(); 
      nit != region->nodes.end(); nit++) {
    Region::Node* n = *nit;
    // For each out edge.
    for (Region::EdgeListItem* i = n->out_edges; i; i=i->next) {
      Region::Edge* edg = i->edge;
      reg << "n" << node_id[edg->src] << " -> " << "n" << node_id[edg->tgt] << ";" << endl;
    }
    // For each in edge.
    for (Region::EdgeListItem* i = n->in_edges; i; i=i->next) {
      Region::Edge* edg = i->edge;
      reg << "n" << node_id[edg->src] << " -> " << "n" << node_id[edg->tgt] << ";" << endl;
    }
  }

  reg << "}" << endl;

}

void RAIn::printRegionsDOT(string& dotf_prefix)
{
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
