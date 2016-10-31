/***************************************************************************
 *   Copyright (C) 2013 by:                                                *
 *   Edson Borin (edson@ic.unicamp.br)                                     *
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
#ifndef RAIN_H
#define RAIN_H

#include <ostream>
#include <map>
#include <list>
#include <set>
#include <unordered_map>

using namespace std;

namespace rain {

  /** 
   *  @brief A Region object represents a region of code.
   */
  class Region {

  public:

    class Edge;

    struct EdgeListItem
    {
      EdgeListItem() : edge(NULL), next(NULL) {}
      Edge* edge;
      EdgeListItem* next;
    };


    /** 
     *  @brief A Region Node object represents one instruction inside the region
     */
    class Node
    {
    public:

      Node();
      Node(unsigned long long);
      ~Node();

      unsigned long long getAddress() {return addr;}

      void insertOutEdge(Edge*, Node*);
      void insertInEdge(Edge*, Node*);

      Edge* findOutEdge(unsigned long long next_ip);
      Edge* findOutEdge(Node* target) const;
      Edge* findInEdge(Node* target) const;

    public:

      unsigned long long  freq_counter; //< Frequency counter.
      Region*  region;       //< Region (NULL for NTE)

      /** Instructions property */
      struct instr_property {
        instr_property() : call(false) {}
        bool     call;         //< Call instruction?
      } inst_property;

      /** List of out edges. */
      EdgeListItem* out_edges;
      EdgeListItem* in_edges;

    private:

      unsigned long long addr; //< Instruction address.
    };

    /** 
     *  @brief A Region Edge represents the control flow between instructions.
     */
    class Edge
    {
    public:

      Edge() {}
      Edge(Node* x, Node* y) : src(x), tgt(y), freq_counter(1) {}

      /// Address of the target instruction.
      unsigned long long target() { return tgt->getAddress(); }

      /// Address of the source instruction.
      unsigned long long source() { return src->getAddress(); }

    public:

      Node* src;                       //< Source node (instruction)
      Node* tgt;                       //< Target node (instruction)
      unsigned long long freq_counter; //< Frequency counter
    };

  public:

    bool alive; // if false, the region has been deleted
    Region() : reg_out_edges(NULL), reg_in_edges(NULL), alive(true) 
    {}
    ~Region();

    unsigned long long allNodesFreq() const;
    unsigned long long entryNodesFreq() const;
    unsigned long long exitNodesFreq() const;
    unsigned long long externalEntriesFreq() const;
    unsigned long long mainExitsFreq() const;

    void insertNode(Node * node)
    {
      node->region = this;
      nodes.insert(node);
    }
    void setEntryNode(Node* node)
    {entry_nodes.insert(node);}
    void setExitNode(Node* node)
    {exit_nodes.insert(node);}
    Edge* createInnerRegionEdge(Node* src, Node* tgt);

    /** A região é composta por nós ligados por arestas, semelhante a um CFG. */
    set<Node* > nodes;
    /** List of pointer to entry nodes. */
    set<Node* > entry_nodes;
    /** List of pointer to exit nodes. */
    set<Node* > exit_nodes;

    Node* getNode(unsigned long long);

    /** Region identifier -- debug purposes. */
    unsigned id;

    void insertRegOutEdge(Edge*);
    void insertRegInEdge(Edge*);

    /** List of pointers to region out edges. */
    EdgeListItem* reg_out_edges;
    /** List of pointers to region in edges. */
    EdgeListItem* reg_in_edges;

    bool isInnerEdge(Edge* e) const;

    /** Region inner edges. */
    list<Edge* > region_inner_edges;

  };

  /** 
   * Region Appraisal Infrastructure class
   * In order to update the state of the trace execution automata (TEA),
   * the user may:
   *
   * // query the edge that will be followed if next addr is executed
   * edg = queryNext(addr);
   * // create a new edge, in case there is no existing edge.
   * if (!edg)
   *   edg = addNext(addr);
   * // then, execute the edge to update the internal state (current node, etc...)
   * executeEdge(edg);
   * 
   */
  class RAIn
  {
  private:

    map<unsigned long long, Region::Edge*> edges;

    /** List of inter region edges. */
    list<Region::Edge* > inter_region_edges;

    unsigned region_id_generator;

  public:

    /** Map region identifiers to regions. */
    map<unsigned, Region*> regions;

    /** NTE node */
    Region::Node* nte;
    /** NTE loop edge. */
    Region::Edge* nte_loop_edge;
    /** NTE out edges map for fast searches. */
    map<unsigned long long, Region::Edge*> nte_out_edges_map;

    /** Current node. */
    Region::Node* cur_node;

    /** Hash table for entry nodes. */
    unordered_map<unsigned long long, Region::Node*> region_entry_nodes;

    RAIn() : region_id_generator(1) // id 0 is reserved for NTE
    {
      nte = new Region::Node(0);
      nte->region = 0;
      nte_loop_edge = new Region::Edge();
      nte_loop_edge->src = nte_loop_edge->tgt = nte;
      cur_node = nte;
    }

    ~RAIn()
    {
      delete nte;
      delete nte_loop_edge;
      for (map<unsigned, Region*>::iterator it = regions.begin(); 
          it != regions.end(); it++)
      {
        Region* r = it->second;
        delete r;
      }
      regions.clear();
    }

    /** Return the edge that will be followed if the next_ip is executed. */
    Region::Edge* queryNext(unsigned long long next_ip);

    /** Add edge supposing next_ip is the next instruction to be executed. 
     *  This should be called only if queryNext has returned NULL. */
    Region::Edge* addNext(unsigned long long next_ip);

    /** Execute the edge (update the current node and related statistics). */
    void executeEdge(Region::Edge* edg);

    /** Create a new region. */
    Region* createRegion();

    /** Create an edge to connect two nodes from different regions. */
    Region::Edge* createInterRegionEdge(Region::Node*, Region::Node*);

    void setEntry(Region::Node *);
    void setExit(Region::Node *);

    void printRegionsStats(ostream&);
    void printOverallStats(ostream&);

    void printRAInStats(ostream&);
    void printRegionDOT(Region *, ostream&);
    void printRegionsDOT(string&);
  };

};

#endif // RAIN_H
