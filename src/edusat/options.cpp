#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "options.h"
using namespace std;


void Abort(string s, int i) {
	cout << endl << "Abort: ";
	switch (i) {
	case 1: cout << "(input error)" << endl; break;
	case 2: cout << "command line arguments error" << endl; break;
	case 3: break;
	default: cout << "(exit code " << i << ")" << endl; break;
	}
	cout << s << endl;
	exit(i);
}

template<typename T> bool Tparse(string st, T val, T lb, T ub, T* var) {
	if (val < lb || val > ub)
		Abort(st + string(" value not in range"), 2);
	*var = val;	
	return true;
}

bool intoption::parse(string st) {
	int val;
	try { val = stoi(st); }
	catch (...) {Abort("value " + st + " not numeric", 1); }
	return Tparse<int>(st, val, lb, ub, p_to_var);
};

bool doubleoption::parse(string st) {
	double val;
	try { val = stod(st); }
	catch (...) { Abort("value " + st + " not double", 1); }
	return Tparse<double>(st, val, lb, ub, p_to_var);
};

extern unordered_map<string, option*> options;

void help() {
	stringstream st;
	st << "\nUsage: edusat <options> <file name>\n \n"
		"Options:\n";
	for (auto h : options) {
		option* opt = options[h.first];
		st << left << setw(16) << "-" + h.first;
		st << opt->msg << ". Default: " << opt->val() << "." << endl;
	}
	Abort(st.str(), 3);
}

int verbose = 0;
double begin_time;
double timeout = 0.0;
VAR_DEC_HEURISTIC VarDecHeuristic = VAR_DEC_HEURISTIC::MINISAT;
VAL_DEC_HEURISTIC ValDecHeuristic = VAL_DEC_HEURISTIC::PHASESAVING;
MODE mode = MODE::INCREMENTAL;

auto o1 = intoption(&verbose, 0, 2, "Verbosity level");
auto o2 = doubleoption(&timeout, 0.0, 36000.0, "Timeout in seconds");
auto o3 = intoption((int*)&ValDecHeuristic, 0, 1, "{0: phase-saving, 1: literal-score}");
auto o4 = intoption((int*)&mode, 0, 1, "{0: normal, 1: incremental}");
unordered_map<string, option*> options = {
    {"v",           &o1},
    {"timeout",     &o2},
    {"valdh",       &o3},
    {"mode",        &o4}
};

void parse_options(int argc, char** argv) {
	if (argc % 2 == 1 || string(argv[1]).compare("-h") == 0)
		help();
	for (int i = 1; i < argc - 1; ++i) {
		
		string st = argv[i] + 1;
		if (argv[i][0] != '-' || options.count(st) == 0) {
			cout << st << endl;
			Abort("Unknown flag ", 2);
		}
		if (i == argc - 2) Abort(string("missing value after ") + st, 2);
		i++;
		options[st]->parse(argv[i]);
	}
	cout << argv[argc - 1] << endl;
}
