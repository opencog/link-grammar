#ifndef __GUIDING_HPP__
#define __GUIDING_HPP__

#include "Solver.h"
#include "util.hpp"

// This class represents different guding strategies of LinkParser SAT search
class Guiding {
public:
  struct SATParameters {
    /* Should the variable with the given number be used as a decision 
       variable during the SAT search? */
    bool isDecision;
    /* What is the decision priority of the variable with the given number
       during the SAT search? */
    double priority;
    /* What is the preffered polarity of the variable with the given number
       during the SAT search? */
    double polarity;
  };

  Guiding(Sentence sent)
    : _sent(sent) {
  }

  /* Abstract functions that calculate params for each type of variable */

  /* string variables */
  virtual void setStringParameters  (int var, const char* str) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }
  virtual void setStringParameters  (int var, const char* str, int cost) = 0;

  /* epsilon variables */
  virtual void setEpsilonParameters (int var) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  /* link_cc variables */
  virtual void setLinkParameters    (int var, int wi, const char* ci, int wj, const char* cj, const char* label) = 0;
  virtual void setLinkParameters    (int var, int wi, const char* ci, int wj, const char* cj, const char* label, 
				     int cost) = 0;

  /* linked_variables */
  virtual void setLinkedParameters  (int var, int wi, int wj) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  virtual void setLinkedMinMaxParameters  (int var, int wi, int wj) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  /* link_cw_variables */
  virtual void setLinkCWParameters  (int var, int wi, int wj, const char* cj) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  /* link_top_cw variables */
  virtual void setLinkTopCWParameters  (int var, int wi, int wj, const char* cj) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  /* link_top_ww variables */
  virtual void setLinkTopWWParameters  (int var, int wi, int wj) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  /* fat_link variables */
  virtual void setFatLinkParameters (int var, int wi, int wj) = 0;
  /* thin_link variables */
  virtual void setThinLinkParameters (int var, int wi, int wj) = 0;
  /* neighbor fat link variables */
  virtual void setNeighborFatLinkParameters (int var, int w) = 0;

  /* Pass SAT search parameters to the MiniSAT solver */
  void passParametersToSolver(Solver* solver) {
    for (int v = 0; v < _parameters.size(); v++) {
      solver->setDecisionVar(v, _parameters[v].isDecision);
      if (_parameters[v].isDecision) {
	solver->setActivity(v, _parameters[v].priority);
	/* TODO: make polarity double instead of boolean*/
	solver->setPolarity(v, _parameters[v].polarity > 0.0);
      }
    }
  }

protected:
  /* Set the parameters for the given variable to given values */
  void setParameters(int var, bool isDecision, double priority, double polarity) {
    if (var >= _parameters.size())
      _parameters.resize(var + 1);
    _parameters[var].isDecision = isDecision;
    _parameters[var].priority = priority;
    _parameters[var].polarity = polarity;

    //    printf("Set: %d %s .%g. .%g.\n", var, isDecision ? "true" : "false", priority, polarity);
  }


  /* Sentence that is being parsed */
  Sentence _sent;

  /* Parameters for each variable */
  std::vector<SATParameters> _parameters;
};

////////////////////////////////////////////////////////////////////////////
class CostDistanceGuiding : public Guiding {
public:
  double cost2priority(int cost) {
    return cost == 0 ? 0.0 : (double)(_sent->length + cost);
  }
  
  CostDistanceGuiding(Sentence sent)
    : Guiding(sent) {
  }

  virtual void setStringParameters  (int var, const char* str, int cost) {
    bool isDecision = cost > 0.0;
    double priority = cost2priority(cost);
    double polarity = 0.0;
    setParameters(var, isDecision, priority, polarity);        
  }

  virtual void setLinkParameters    (int var, int wi, const char* ci, int wj, const char* cj, const char* label) {
    bool isDecision = true;
    double priority = 0.0;
    double polarity = 0.0;
    setParameters(var, isDecision, priority, polarity);    
  }

  virtual void setLinkParameters(int var, int i, const char* ci, int j, const char* cj, const char* label, 
				 int cost)  {
    bool isDecision = true;
    double priority = cost2priority(cost);
    double polarity = 0.0;
    setParameters(var, isDecision, priority, polarity);
  }

  void setFatLinkParameters(int var, int i, int j) {
    bool isDecision = true;
    double priority = (double)abs(j - i);

    if (_sent->is_conjunction[i])
      priority = 0.0;

    double polarity = abs(j - i) == 1 ? 1.0 : 0.0;
    setParameters(var, isDecision, priority, polarity);
  }

  void setThinLinkParameters(int var, int i, int j) {
    bool isDecision = true;

    double priority = (double)(j - i);
    if (_sent->is_conjunction[i] || _sent->is_conjunction[j])
      priority = priority / 2;

    double polarity = j - i == 1 ? 1.0 : 0.0;
    if (i == 0 && j == _sent->length - 2 && isEndingInterpunction(_sent->word[j].string)) {
      polarity = 1.0;
    }
    if (i == 0 && j == _sent->length - 1 && !isEndingInterpunction(_sent->word[j - 1].string)) {
      polarity = 1.0;
    }

    setParameters(var, isDecision, priority, polarity);
  }

  void setNeighborFatLinkParameters(int var, int w) {
    bool isDecision = true;
    double priority = _sent->length;
    double polarity = 1.0;
    setParameters(var, isDecision, priority, polarity);    
  }

};



////////////////////////////////////////////////////////////////////////////
class CostDistanceGuidingOnlyLink : public Guiding {
public:
  double cost2priority(int cost) {
    return cost == 0 ? 0.0 : (double)(_sent->length + cost);
  }
  
  CostDistanceGuidingOnlyLink(Sentence sent)
    : Guiding(sent) {
  }

  virtual void setStringParameters  (int var, const char* str, int cost) {
    bool isDecision = cost > 0.0;
    double priority = cost2priority(cost);
    double polarity = 0.0;
    setParameters(var, isDecision, priority, polarity);        
  }

  virtual void setLinkParameters    (int var, int wi, const char* ci, int wj, const char* cj, const char* label) {
    bool isDecision = true;
    double priority = _sent->length - (wj - wi);
    double polarity = wj - wi <= 3 ? 1.0 : 0.0;;
    setParameters(var, isDecision, priority, polarity);    
  }

  virtual void setLinkParameters(int var, int wi, const char* ci, int wj, const char* cj, const char* label, 
				 int cost)  {
    bool isDecision = true;
    double priority = cost > 0 ? cost2priority(cost) : wj - wi;
    double polarity = wj - wi <= 3 ? 1.0 : 0.0;
    setParameters(var, isDecision, priority, polarity);
  }

  void setFatLinkParameters(int var, int i, int j) {
    bool isDecision = true;
    double priority = (double)abs(j - i);

    if (_sent->is_conjunction[i])
      priority = 0.0;

    double polarity = abs(j - i) == 1 ? 1.0 : 0.0;
    setParameters(var, isDecision, priority, polarity);
  }

  void setThinLinkParameters(int var, int i, int j) {
    bool isDecision = false;
    setParameters(var, isDecision, 0.0, 0.0);
  }

  void setNeighborFatLinkParameters(int var, int w) {
    bool isDecision = true;
    double priority = _sent->length;
    double polarity = 1.0;
    setParameters(var, isDecision, priority, polarity);    
  }

};
#endif
