#include <cassert>
#include <iostream>
#include "edusat/ipasir.h"


typedef void* Solver;

int main() {
    std::cout << "Starting test...";
    Solver s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);
    assert(ipasir_solve(s) == 10);
    assert(ipasir_val(s, 1) == 1 || ipasir_val(s, 2) == 2);
    ipasir_release(s);
    std::cout << "Success!" << std::endl;
    return 0;
}
