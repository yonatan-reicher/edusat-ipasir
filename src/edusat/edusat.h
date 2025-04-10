#pragma once
#include <iostream>
#include <iterator>
#include <vector>
#include <unordered_set>
#include <map>
#include <string>
#include <fstream>
#include <cassert>
#include <ctime>
#include "options.h"
using namespace std;

typedef int Var;
typedef int Lit;
typedef vector<Lit> clause_t;
typedef clause_t::iterator clause_it;
typedef vector<Lit> trail_t;

#define Assert(exp) AssertCheck(exp, __func__, __LINE__)


#define Neg(l) (l & 1)
#define Restart_multiplier 1.1f
#define Restart_lower 100
#define Restart_upper 1000
#define Max_bring_forward 10
#define var_decay 0.99
#define Rescale_threshold 1e100
#define Assignment_file "assignment.txt"

void Abort(string s, int i);

enum class LitState {
	L_UNSAT,
	L_SAT,
	L_UNASSIGNED
};

enum class VarState {
	V_FALSE,
	V_TRUE,
	V_UNASSIGNED
};

enum class ClauseState {
	C_UNSAT,
	C_SAT,
	C_UNIT,
	C_UNDEF
};

enum class SolverState{
	UNSAT,
	SAT,
	CONFLICT, 
	UNDEF,
	TIMEOUT
} ;
/***************** service functions **********************/

#include <ctime>

static inline double cpuTime(void) {
    return (double)clock() / CLOCKS_PER_SEC; }

// For production wrap with #ifdef _DEBUG
inline void AssertCheck(bool cond, string func_name, int line, string msg = "") {
	if (cond) return;
	cout << "Assertion fail" << endl;
	cout << msg << endl;
	cout << func_name << " line " << line << endl;
	exit(1);
}


inline bool match(ifstream& in, const char* str) {
    for (; *str != '\0'; ++str)
        if (*str != in.get())
            return false;
    return true;
}

inline unsigned int Abs(int x) { // because the result is compared to an unsigned int. unsigned int are introduced by size() functions, that return size_t, which is defined to be unsigned. 
	if (x < 0) return (unsigned int)-x;
	else return (unsigned int)x;
}

inline unsigned int v2l(int i) { // maps a literal as it appears in the cnf to literal
	if (i < 0) return ((-i) << 1) - 1; 
	else return i << 1;
} 

inline Var l2v(Lit l) {
	return (l+1) / 2;	
} 

inline Lit negate_(Lit l) {
	if (Neg(l)) return l + 1;  // odd
	return l - 1;		
}

inline int l2rl(int l) {
	return Neg(l)? -((l + 1) / 2) : l / 2;
}


/********** classes ******/ 

class Clause {
	clause_t c;
	int lw,rw; //watches;	
public:	
	Clause(){};
	void insert(int i) {c.push_back(i);}
	void lw_set(int i) {lw = i; /*assert(lw != rw);*/}
	void rw_set(int i) {rw = i; /*assert(lw != rw);*/}	
	clause_t& cl() {return c;}
	int get_lw() {return lw;}
	int get_rw() {return rw;}
	int get_lw_lit() {return c[lw];}
	int get_rw_lit() {return c[rw];}
	int  lit(int i) {return c[i];} 		
	inline ClauseState next_not_false(bool is_left_watch, Lit other_watch, bool binary, int& loc); 
	size_t size() {return c.size();}
	void reset() { c.clear(); }	
	void print() {for (clause_it it = c.begin(); it != c.end(); ++it) {cout << *it << " ";}; }
	void print_real_lits() {
		Lit l; 
		cout << "("; 
		for (clause_it it = c.begin(); it != c.end(); ++it) { 
			l = l2rl(*it); 
			cout << l << " ";} cout << ")"; 
	}
	void print_with_watches() {		
		for (clause_it it = c.begin(); it != c.end(); ++it) {
			cout << l2rl(*it);
			int j = distance(c.begin(), it); //also could write "int j = i - c.begin();"  : the '-' operator is overloaded to allow such things. but distance is more standard, as it works on all standard containers.
			if (j == lw) cout << "L";
			if (j == rw) cout << "R";
			cout << " ";
		};
	}
};

struct Solver {
	vector<Clause> cnf; // clause DB. 
	vector<int> unaries; 
	trail_t trail;  // assignment stack	
	vector<int> separators; // indices into trail showing increase in dl 	
	vector<int> LitScore; // literal => frequency of this literal (# appearances in all clauses). 
	vector<vector<int> > watches;  // Lit => vector of clause indices into CNF
	vector<VarState> state;  // current assignment
	vector<VarState> prev_state; // for phase-saving: same as state, only that it is not reset to 0 upon backtracking. 
	vector<int> antecedent; // var => clause index in the cnf vector. For variables that their value was assigned in BCP, this is the clause that gave this variable its value. 
	vector<bool> marked;	// var => seen during analyze()
	vector<int> dlevel; // var => decision level in which this variable was assigned its value. 
	vector<int> conflicts_at_dl; // decision level => # of conflicts under it. Used for local restarts. 

	// Used by VAR_DH_MINISAT:	
    vector<bool> m_HasVarBeenPutInScore2Vars;
	map<double, unordered_set<Var>, greater<double>> m_Score2Vars; // 'greater' forces an order from large to small of the keys
	map<double, unordered_set<Var>, greater<double>>::iterator m_Score2Vars_it;
	unordered_set<Var>::iterator m_VarsSameScore_it;
	vector<double>	m_activity; // Var => activity
	double			m_var_inc = 0;	// current increment of var score (it increases over time)
	double			m_curr_activity = 0;
	bool			m_should_reset_iterators = 0;

	unsigned int 
		nvars = 0,			// # vars
		nclauses = 0, 		// # clauses
		nlits = 0,			// # literals = 2*nvars				
		qhead = 0;			// index into trail. Used in BCP() to follow the propagation process.
	int					
		num_learned = 0, 	
		num_decisions = 0,
		num_assignments = 0,
		num_restarts = 0,
		dl = 0,				// decision level
		max_dl = 0,			// max dl seen so far since the last restart
		conflicting_clause_idx = 0, // holds the index of the current conflicting clause in cnf[]. -1 if none.				
		restart_threshold = 0,
		restart_lower = 0,
		restart_upper = 0;

	Lit 		asserted_lit = 0;

	float restart_multiplier = 0;

    int (*terminate_callback)(void*) = nullptr;
    void* terminate_callback_state = nullptr;

    void (*learn_callback)(void*, int*) = nullptr;
    void* learn_callback_state = nullptr;
    int learn_callback_max_length = 0;

    std::unordered_set<int> indices_of_temporary_assertions;
	
	// access	
	int get_learned() { return num_learned; }
	void set_nvars(int x) { nvars = x; }
	int get_nvars() { return nvars; }
	void set_nclauses(int x) { nclauses = x; }
	size_t cnf_size() { return cnf.size(); }
	VarState get_state(int x) { return state[x]; }

	// misc.
	void add_to_trail(int x) { trail.push_back(x); }

	void make_space_for_vars();
	void reset(); // initialization that is invoked initially + every restart
	void initialize();
	void reset_iterators(double activity_key = 0.0);	

	// solving	
	SolverState decide();
	void test();
	SolverState BCP();
	int  analyze(const Clause&);
	inline int  getVal(Var v);
	void add_clause(Clause& c, int l, int r);
	void add_unary_clause(Lit l);
	void assert_lit(Lit l);	
	void temporary_assert(Lit l);	
    void unassert_temporaries();
	void m_rescaleScores(double& new_score);
	inline void backtrack(int k);
	void restart();
	
	// scores	
	void bumpVarScore(int idx);
	void bumpLitScore(int lit_idx);

public:
	Solver(): 
		nvars(0), nclauses(0), num_learned(0), num_decisions(0), num_assignments(0), 
		num_restarts(0), m_var_inc(1.0), qhead(0), 
		restart_threshold(Restart_lower), restart_lower(Restart_lower), 
		restart_upper(Restart_upper), restart_multiplier(Restart_multiplier)	 {};
	
	// service functions
	inline LitState lit_state(Lit l) {
		VarState var_state = state[l2v(l)];
		return var_state == VarState::V_UNASSIGNED ? LitState::L_UNASSIGNED : (Neg(l) && var_state == VarState::V_FALSE || !Neg(l) && var_state == VarState::V_TRUE) ? LitState::L_SAT : LitState::L_UNSAT;
	}
	inline LitState lit_state(Lit l, VarState var_state) {
		return var_state == VarState::V_UNASSIGNED ? LitState::L_UNASSIGNED : (Neg(l) && var_state == VarState::V_FALSE || !Neg(l) && var_state == VarState::V_TRUE) ? LitState::L_SAT : LitState::L_UNSAT;
	}
	void read_cnf(ifstream& in);

	SolverState _solve();
	void solve();

	
	
	
// debugging
	void print_cnf(){
		for(vector<Clause>::iterator i = cnf.begin(); i != cnf.end(); ++i) {
			i -> print_with_watches(); 
			cout << endl;
		}
	} 

	void print_real_cnf() {
		for(vector<Clause>::iterator i = cnf.begin(); i != cnf.end(); ++i) {
			i -> print_real_lits(); 
			cout << endl;
		}
	} 

	void print_state(const char *file_name) {
		ofstream out;
		out.open(file_name);		
		out << "State: "; 
		for (vector<VarState>::iterator it = state.begin() + 1; it != state.end(); ++it) {
			char sign = (*it) == VarState::V_FALSE ? -1 : (*it) == VarState::V_TRUE ? 1 : 0;
			out << sign * (it - state.begin()) << " "; out << endl;
		}
	}	

	void print_state() {
		cout << "State: "; 
		for (vector<VarState>::iterator it = state.begin() + 1; it != state.end(); ++it) {
			char sign = (*it) == VarState::V_FALSE ? -1 : (*it) == VarState::V_TRUE ? 1 : 0;
			cout << sign * (it - state.begin()) << " "; cout << endl;
		}
	}	
	
	void print_watches() {
		for (vector<vector<int> >::iterator it = watches.begin() + 1; it != watches.end(); ++it) {
			cout << distance(watches.begin(), it) << ": ";
			for (vector<int>::iterator it_c = (*it).begin(); it_c != (*it).end(); ++it_c) {
				cnf[*it_c].print();
				cout << "; ";
			}
			cout << endl;
		}
	};


	void print_stats() {cout << endl << "Statistics: " << endl << "===================" << endl << 
		"### Restarts:\t\t" << num_restarts << endl <<
		"### Learned-clauses:\t" << num_learned << endl <<
		"### Decisions:\t\t" << num_decisions << endl <<
		"### Implications:\t" << num_assignments - num_decisions << endl <<
		"### Time:\t\t" << cpuTime() - begin_time << endl;
	}
	
	void validate_assignment();
};


extern Solver S;
