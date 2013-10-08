/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#include "perftest.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    Clockrate clockrate(4, 0.25);
    for(int i = 0; i < 4; i++)
        clockrate.sample();
    std::cout << clockrate.rate()/1e+9 << "GHz" << std::endl;
}
