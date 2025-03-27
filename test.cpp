#include <chrono>
#include <functional>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
#include "src/ipasir.h"


using namespace std;
using duration = chrono::duration<double, milli>;


typedef void* Solver;



vector<const char*> summary;


#define TEST(test_name) \
    run_test( \
        #test_name, \
        test_##test_name, \
        "Running test " #test_name "...", \
        u8"✅ " #test_name " passed", \
        u8"❌ " #test_name " failed" \
    )
void run_test(
    const char* test_name,
    void (*test_function)(),
    const char* intro,
    const char* success,
    const char* failure
) {
    try {                                                                     
        cout << intro << endl;
        test_function();
        std::cout << success << std::endl;               
        summary.push_back(success);
    } catch (const std::exception& e) {                                       
        std::cout << failure << ": " << e.what() << std::endl; 
        summary.push_back(failure);                               
    }
}


#define TEST_SUMMARY() print_test_summary()
void print_test_summary() {
    cout << "Summary:" << endl;
    for (const auto &line : summary) cout << line << endl;
}


#define ASSERT(condition, msg)                                              \
    if (!(condition)) {                                                     \
        throw std::runtime_error(                                           \
            "Assertion failed: " #condition "\n" msg                        \
        );                                                                  \
    }


Solver solver_from_string(const std::string& string) {
    Solver s = ipasir_init();
    stringstream ss(string);
    while (true) {
        int x; ss >> x;
        if (ss.eof()) break;
        ipasir_add(s, x);
    }
    return s;
}


duration measure_time(function<void()> f) {
    auto start = chrono::high_resolution_clock::now();
    f();
    auto end = chrono::high_resolution_clock::now();
    return end - start;
}


void test_execution_time_decreases() {
    Solver s = solver_from_string(R"(
         1  2  3  4  0
        -1  2  3 -4  0
        -1  2  0
        -1  3  0
        -4  3  2  0
    )");
    int res_1 = -1;
    int res_2 = -1;
    cout << "First solve: ";
    duration dur_1 = measure_time([&] { res_1 = ipasir_solve(s); });
    cout << res_1 << ", " << dur_1.count() << "ms" << endl;
    cout << "Second solve: ";
    duration dur_2 = measure_time([&] { res_2 = ipasir_solve(s); });
    cout << res_2 << ", " << dur_2.count() << "ms" << endl;
    ipasir_release(s);
    ASSERT(res_1 == 10, "Solving (first time) must succeed");
    ASSERT(res_2 == 10, "Solving (second time) must succeed");
    ASSERT(dur_2 < dur_1, "Execution time must decrease (incrementality)");
}


vector<int> solver_get_assignment(Solver s, int max_var) {
    vector<int> ret(max_var + 1);
    for (int i = 1; i <= max_var; i++) {
        ret[i] = ipasir_val(s, i);
    }
    return ret;
}


string pretty_print_assignment(const vector<int>& a) {
    string ret;
    bool did_first_print = false;
    for (int i = 1; i < a.size(); i++) {
        int val = a[i];
        ASSERT(val == 0 || abs(val) == i, "Assignment is invalid");
        string s = to_string(val);

        if (val == 0) continue;
        if (did_first_print) {
            ret += " ";
            ret += s;
        } else {
            ret += s;
            did_first_print = true;
        }
    }
    return ret;
}


// Obviously, this only works on specific problems.
void test_flipped_assignment_is_wrong() {
    constexpr int VARS = 3;
    const std::string problem = R"(
        1 2 0
        2 0
        3 0
        1 2 3 0
    )";
    Solver s = solver_from_string(problem);
    vector<int> values;

    cout << "Starting first solve" << endl;
    int res = ipasir_solve(s);
    ASSERT(res == 10, "Solving should succeed");
    values = solver_get_assignment(s, VARS);
    ipasir_release(s);

    cout << "Found assignment ";
    cout << pretty_print_assignment(values) << endl;

    cout << "Assuming flipped assignment" << endl;
    s = solver_from_string(problem);
    for (int i = 1; i <= VARS; i++) {
        if (values[i] == 0) continue;
        ipasir_assume(s, -values[i]);
    }
    cout << "Starting second solve" << endl;
    res = ipasir_solve(s);
    ASSERT(res == 20, "Solving with flipped assignment should fail");
}

// Test that assume adds assumptions correctly and reuses work.
void test_assume_for_incrementality() {
    // This problem is a 4✗4 grid. Each row must have exactly one true cell,
    // and so does each column.
    const std::string problem = R"(
          1   2   3   4   0
          5   6   7   8   0
          9  10  11  12   0
         13  14  15  16   0

         -1  -2   0
         -1  -3   0
         -1  -4   0
         -2  -3   0
         -2  -4   0
         -3  -4   0

         -5  -6   0
         -5  -7   0
         -5  -8   0
         -6  -7   0
         -6  -8   0
         -7  -8   0
         
         -9 -10   0
         -9 -11   0
         -9 -12   0
        -10 -11   0
        -10 -12   0
        -11 -12   0

        -13 -14   0
        -13 -15   0
        -13 -16   0
        -14 -15   0
        -14 -16   0
        -15 -16   0

         -1  -5   0
         -1  -9   0
         -1 -13   0
         -5  -9   0
         -5 -13   0
         -9 -13   0

         -2  -6   0
         -2 -10   0
         -2 -14   0
         -6 -10   0
         -6 -14   0
        -10 -14   0

         -3  -7   0
         -3 -11   0
         -3 -15   0
         -7 -11   0
         -7 -15   0
        -11 -15   0

         -4  -8   0
         -4 -12   0
         -4 -16   0
         -8 -12   0
         -8 -16   0
        -12 -16   0
    )";
    Solver s = solver_from_string(problem);

    cout << "First solving" << endl;
    int res = ipasir_solve(s);
    ASSERT(res == 10, "Solving should succeed");
    vector<int> assignment = solver_get_assignment(s, 16);
    cout << "Found assignment " << pretty_print_assignment(assignment) << endl;

    assignment[4] *= -1;
    cout << "Solving with assumption " << assignment[4] << endl;
    ipasir_assume(s, assignment[4]);
    res = ipasir_solve(s);
    ASSERT(res == 10, "Solving should succeed");
    ASSERT(ipasir_val(s, assignment[4]) == assignment[4], "Assumption");
    cout << "Found assignment "
        << pretty_print_assignment(solver_get_assignment(s, 16)) << endl;

    assignment[7] *= -1;
    cout << "Solving with assumption " << assignment[4] << " "
        << assignment[7] << endl;
    ipasir_assume(s, assignment[4]);
    ipasir_assume(s, assignment[7]);
    res = ipasir_solve(s);
    ASSERT(res == 10, "Solving should succeed");
    ASSERT(ipasir_val(s, assignment[4]) == assignment[4], "Assumption");
    ASSERT(ipasir_val(s, assignment[7]) == assignment[7], "Assumption");
    cout << "Found assignment "
        << pretty_print_assignment(solver_get_assignment(s, 16)) << endl;
}


void test_assume_dissappears() {
    Solver s = solver_from_string(R"(
        1 2 3 0
        -1 -2 0
    )");

    cout << "First solve:" << endl;
    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    int res = ipasir_solve(s);
    ASSERT(res == 10, "Should succeed");
    vector<int> assignment = solver_get_assignment(s, 3);
    ASSERT(assignment[1] == -1, "Assignment does not make since");
    ASSERT(assignment[2] == -2, "Assignment does not make since");
    ASSERT(assignment[3] == 3, "Assignment does not make since");

    cout << "Second solve:" << endl;
    ipasir_assume(s, -3);
    res = ipasir_solve(s);
    ASSERT(res == 10, "There should be an assignment because the previous "
            "assumptions disappeared");

    cout << "Third solve:" << endl;
    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    ipasir_assume(s, -3);
    res = ipasir_solve(s);
    ASSERT(res == 20, "But for this, there should not be a solution");
}


int main() {
#ifdef _WIN32
    // We use utf8 characters
    SetConsoleOutputCP(CP_UTF8);
#endif

    cout << "Starting" << endl;

    TEST(execution_time_decreases);
    TEST(flipped_assignment_is_wrong);
    TEST(assume_for_incrementality);
    TEST(assume_dissappears);

    cout << "End" << endl;
    cout  << endl;

    TEST_SUMMARY();

    return 0;
}
