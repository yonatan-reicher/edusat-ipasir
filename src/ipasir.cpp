#include "ipasir.h"
#include "edusat/edusat.h"

Clause clause;


/**
 * This function takes a cnf-like literal ( var / -var ) and returns it as
 * a `Lit` - and also creates it in the S if it does not exist!
 */
Lit literal(int lit) {
    if (abs(lit) > S.get_nvars()) {
        S.set_nvars(abs(lit));
        S.make_space_for_vars();
        int i = lit;
		if (VarDecHeuristic == VAR_DEC_HEURISTIC::MINISAT) {
            S.bumpVarScore(abs(i));
        }
		i = v2l(i);		
		if (ValDecHeuristic == VAL_DEC_HEURISTIC::LITSCORE) {
            S.bumpLitScore(i);
        }
    }
    return v2l(lit);
}


IPASIR_API const char * ipasir_signature () {
    return "edusat";
}


IPASIR_API void * ipasir_init () {
    S = Solver();
    S.initialize();
    return nullptr;
}


IPASIR_API void ipasir_release (void * state) {
}


IPASIR_API void ipasir_add (void * state, int lit_or_zero) {
    if (lit_or_zero == 0) { // Clause finished!
        switch (clause.size()) {
            case 0:
                throw std::logic_error("Empty clause!");
            case 1: {
                int lit = clause.lit(0);
                S.add_unary_clause(lit);
                // TODO: Do we need to re-assert this after every
                // restart/reset?
                S.assert_lit(lit);
                break;
            }
            default:
                S.add_clause(clause, 0, 1);
                break;
        }
        clause = Clause();
    } else {
        clause.insert(literal(lit_or_zero));
    }
}


IPASIR_API void ipasir_assume (void * state, int lit) {
    S.assert_lit(literal(lit));
}


IPASIR_API int ipasir_solve (void * state) {
	if (VarDecHeuristic == VAR_DEC_HEURISTIC::MINISAT) {
        S.reset_iterators();
    }
    switch (S._solve()) {
        case SolverState::SAT:
            return 10;
        case SolverState::UNSAT:
            return 20;
        case SolverState::TIMEOUT:
            return 0;
        default:
            throw std::logic_error("Invalid result!");
    }
}


IPASIR_API int ipasir_val (void * state, int lit) {
    literal(lit);
    switch (S.state[abs(lit)]){
        case VarState::V_FALSE:
            return -abs(lit);
        case VarState::V_TRUE:
            return abs(lit);
        default:
            return 0;
    }
}


IPASIR_API int ipasir_failed (void * state, int lit) {
    literal(lit);
    return S.state[abs(lit)] != VarState::V_UNASSIGNED;
}


IPASIR_API void ipasir_set_terminate (void * solverState, void * state, int (*terminate)(void * state)) {
    S.terminate_callback_state = state;
    S.terminate_callback = terminate;
}


IPASIR_API void ipasir_set_learn (void * state, void * learnState, int max_length, void (*learn)(void * state, int * clause)) {
    S.learn_callback_state = learnState;
    S.learn_callback_max_length = max_length;
    S.learn_callback = learn;
}
