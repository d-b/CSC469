/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#include "perftest.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::vector<period_t> samples;
    u_int64_t start = inactive_periods(5, 20000, samples);
    return 0;
}
