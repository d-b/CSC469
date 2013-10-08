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

void usage(const char* application) {
    std::cout << "Usage: " << application
              << " [-n <samples>] [-t <threshold>] [-o <output file>]" << std::endl;
}

int main(int argc, char* argv[]) {
    // Sanity check
    if(argc <= 0) return -1;

    // Defaults
    int samplecount = 10;
    u_int64_t threshold = 10000;
    std::string outputpath;

    // Process arguments
    int option;
    while ((option = getopt (argc, argv, "n:t:o:")) != -1)
        switch (option) {
            case 'n':
            samplecount = atoi(optarg);
            break;

            case 't':
            threshold = atoi(optarg);
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
    
    // Write results to file if required
    if(!outputpath.empty()) {
        std::ofstream stream(outputpath);
        stream << "{\"start\": " << start << "," << std::endl
               << " \"threshold\": " << threshold << "," << std::endl
               << " \"samples\": [" << std::endl;
        
        for(auto iter = samples.begin(); iter != samples.end(); iter++) {
            stream << "    {\"start\": " << iter->start
                   << ", \"end\": "      << iter->end
                   << "}," << std::endl;
        }
        
        stream << "]}" << std::endl;
        stream.close();
    }

    return 0;
}
