#include "ipasir.h"
#include "edusat/edusat.h"

Clause clause;
bool has_been_reset = true;


#ifdef EDUSAT_DEBUG
#define DBG(expr)                                                           \
    ( std::cout << __func__ << " " << #expr "=" << (expr) << std::endl      \
    , (expr)                                                                \
    )
#else
#define DBG(expr) expr
#endif


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


void check_reset() {
    if (!has_been_reset) {
        S.restart();
        S.unassert_temporaries();
        for (Lit l : S.unaries) { S.assert_lit(l); }
        has_been_reset = true;
    }
}


IPASIR_API const char * ipasir_signature () {
    return DBG("edusat");
}


IPASIR_API void * ipasir_init () {
    S = Solver();
    S.initialize();
#ifdef EDUSAT_VERBOSE
    verbose = EDUSAT_VERBOSE;
#endif
    return DBG(nullptr);
}


IPASIR_API void ipasir_release (void * state) {
    S = Solver();
    clause = Clause();
}


IPASIR_API void ipasir_add (void * state, int lit_or_zero) {
    DBG(lit_or_zero);
    check_reset();
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
    DBG(lit);
    check_reset();
    // Temporary assertions are un-assumed when resetting the model.
    S.temporary_assert(literal(lit));
}


Var find_bad_var() {
    vector<VarState> state(S.state.size(), VarState::V_UNASSIGNED);
    for (Lit l : S.trail) {
        auto v = Neg(l) ? VarState::V_FALSE : VarState::V_TRUE;
        auto* cell = &state[l2v(l)];
        if (*cell != VarState::V_UNASSIGNED && *cell != v) return l2v(l);
        *cell = v;
    }
    return 0;
}


IPASIR_API int ipasir_solve (void * state) {
    has_been_reset = false;
	if (VarDecHeuristic == VAR_DEC_HEURISTIC::MINISAT) {
        S.reset_iterators();
    }
    // Must first check for bad assumptions!
    if (Var bad_var = find_bad_var()) {
        S.assert_lit(v2l(bad_var));
        return DBG(20);
    }
    switch (S._solve()) {
        case SolverState::SAT:
            return DBG(10);
        case SolverState::UNSAT:
            return DBG(20);
        case SolverState::TIMEOUT:
            return DBG(0);
        default:
            throw std::logic_error("Invalid result!");
    }
}


IPASIR_API int ipasir_val (void * state, int lit) {
    DBG(lit);
    literal(lit);
    switch (S.state[abs(lit)]){
        case VarState::V_FALSE:
            return DBG(-abs(lit));
        case VarState::V_TRUE:
            return DBG(abs(lit));
        default:
            return 0;
    }
}


IPASIR_API int ipasir_failed (void * state, int lit) {
    DBG(lit);
    literal(lit);
    return DBG(S.state[abs(lit)] != VarState::V_UNASSIGNED);
}


IPASIR_API void ipasir_set_terminate (void * solverState, void * state, int (*terminate)(void * state)) {
    DBG(state);
    S.terminate_callback_state = state;
    S.terminate_callback = terminate;
}


IPASIR_API void ipasir_set_learn (void * state, void * learnState, int max_length, void (*learn)(void * state, int * clause)) {
    DBG(max_length);
    S.learn_callback_state = learnState;
    S.learn_callback_max_length = max_length;
    S.learn_callback = learn;
}
