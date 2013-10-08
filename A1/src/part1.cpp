/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#include "perftest.hpp"

// System includes
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <iostream>
#include <iomanip>

void usage(const char* application) {
    std::cout << "Usage: " << application
              << " [-n <samples>] [-t <threshold>] [-s silent] [-o <output file>]" << std::endl;
}

int main(int argc, char* argv[]) {
    // Sanity check
    if(argc <= 0) return -1;

    // Defaults
    int samplecount = 10;
    u_int64_t threshold = 10000;
    bool silent = false;
    std::string outputpath;

    // Process arguments
    int option;
    while ((option = getopt (argc, argv, "n:t:so:")) != -1)
        switch (option) {
            case 'n':
            samplecount = atoi(optarg);
            break;

            case 't':
            threshold = atoi(optarg);
            break;

            case 's':
            silent = true;
            break;

            case 'o':
            outputpath = optarg;
            break;

            default:
                usage(argv[0]);
                return -1;
        }

    // Perform the experiment
    std::vector<period_t> samples;
    u_int64_t start = inactive_periods(samplecount, threshold, samples);

    // Get the clockrate
    Clockrate clocksampler(4, 0.25);
    for(int i = 0; i < 4; i++)
        clocksampler.sample();
    Clockrate::hertz clockrate = clocksampler.rate();

    // Output the results
    if(!silent) {
        std::cout << "Measured system frequency: " << clockrate/1e+6 << "MHz" << std::endl;
        static const char* period_type[]  = {"Active", "Inactive"};
        period_t active = {start, start};
        int i; for(auto iter = samples.begin(); iter != samples.end(); iter++, i++) {
            period_t& inactive = *iter;
            period_t* period_array[] = {&active, &inactive};
            active.end = iter->start;

            for(int j = 0; j < 2; j++) {
                period_t& period = *period_array[j];
                u_int64_t duration = period.end - period.start;
                std::cout << period_type[j] << " " << i << ": start at " << period.start
                          << ", duration " << duration
                          << " cycles (" << std::setprecision(6) << std::fixed
                          << ((double)duration/clockrate)*1e+3
                          << " ms)" << std::endl;
            }

            active.start = iter->end;
        }
    }

    // Write results to file if required
    if(!outputpath.empty()) {
        // Write header
        std::ofstream stream(outputpath);
        stream << "{\"start\": " << start << "," << std::endl
               << " \"threshold\": " << threshold << "," << std::endl
               << " \"frequency\": " << clockrate << "," << std::endl
               << " \"samples\": [" << std::endl;

        // Write samples
        for(auto iter = samples.begin(); iter != samples.end(); iter++)
            stream << "    {\"start\": " << iter->start
                   << ", \"end\": "      << iter->end
                   << ((iter + 1 != samples.end()) ? "}," : "}")
                   << std::endl;

        // Write footer & close file
        stream << "]}" << std::endl;
        stream.close();
    }

    return 0;
}
