#pragma once
#include <string>
using namespace std;

class option {
public:
	option(string _msg) : msg(_msg) {};	
	string msg;
	virtual bool parse(string) = 0; 
	virtual string val() = 0; 
};

class intoption : public option {
public:	intoption(int* p, int _lb, int _ub, string _msg) : option(_msg),
	p_to_var(p), lb(_lb), ub(_ub) {};
	  int* p_to_var; // pointer to the variable holding the option value. 
	  int lb; // lower-bound
	  int ub; // upper-bound
	  bool parse(string st); 
	  string val() { return to_string(*p_to_var); }
};

class doubleoption : public option {
public:	doubleoption(double* p, double _lb, double _ub, string _msg) : option(_msg),
	p_to_var(p), lb(_lb), ub(_ub) {}
	  double* p_to_var; // pointer to the variable holding the option value. 
	  double lb; // lower-bound
	  double ub; // upper-bound
	  bool parse(string st);
	  string val() { return to_string(*p_to_var); }
};
void Abort(string s, int i);
void parse_options(int argc, char** argv);

enum class VAR_DEC_HEURISTIC {
    MINISAT
    // add other decision heuristics here. Add an option to choose between them.
 } ;

// Heuristics for choosing values for variables when faced with a decision.
enum class VAL_DEC_HEURISTIC {
    /* Same as last value. Initially false*/
    PHASESAVING, 
    /* Choose literal with highest frequency */
    LITSCORE 
};

enum class MODE: int {
    NORMAL = 0,
    INCREMENTAL = 1,
};


extern int verbose;
extern double begin_time;
extern double timeout;
extern VAR_DEC_HEURISTIC VarDecHeuristic;
extern VAL_DEC_HEURISTIC ValDecHeuristic;
extern MODE mode;
