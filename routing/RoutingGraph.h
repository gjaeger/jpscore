/*
 * GraphRouter.h
 *
 *  Created on: Aug 20, 2012
 *      Author: David Haensel
 */

#ifndef ROUTINGGRAPH_H_
#define ROUTINGGRAPH_H_

#include <vector>
#include <map>
#include "../geometry/Transition.h"
#include "../geometry/Crossing.h"
#include "../geometry/SubRoom.h"
struct exit_distance;
struct vertex;
struct edge;


class RoutingGraph {

 private:


  const vector<Crossing*> * crossings;
  const vector<Transition*> * transitions;
  map<int, vertex> vertexes;
  void processSubroom(SubRoom * sub, map<int, vertex> & vertexes, Crossing * crossing);
  void processNewCrossingEdge(Crossing * new_crossing, Crossing * act_crossing);
  void calculateDistances(vertex * exit, vertex * last_vertex, vertex * act_vertex, double act_distance);
 public:
  RoutingGraph();
  RoutingGraph(const vector<Transition*> &trans , const vector<Crossing*> &cross);
  virtual ~RoutingGraph();
  void print();

 protected:
  RoutingGraph* BuildGraph();


};



#endif /* ROUTINGGRAPH_H_ */