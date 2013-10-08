/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#include "perftest.hpp"

// System includes
#include <time.h>

/*
 * Clockrate::sample
 *
 * Collect a clockrate sample and add it to the buffer
 */
void Clockrate::sample() {
    // Requested sleep time
    timespec req, rem;
    req.tv_sec  = (time_t) sample_seconds;
    req.tv_nsec = (long) ((sample_seconds - req.tv_sec) * 1e+9);

    // Capture sample
    TSC counter;
    counter.start();
    nanosleep(&req, &rem);
    u_int64_t cycles = counter.count();
    hertz clockrate = (hertz) cycles / sample_seconds;

    // See if we need to pop off an existing sample
    if(sample_buffer.size() >= sample_count) {
        auto iter = sample_buffer.begin();
        sample_total -= *iter;
        sample_buffer.erase(iter);
    }

    // Add new sample to the buffer
    sample_buffer.push_back(clockrate);
    sample_total += clockrate;
}

/*
 * Clockrate::rate
 *
 * Return the current approximation of the clock rate
 */
Clockrate::hertz Clockrate::rate() {
    size_t count = sample_buffer.size();
    return (hertz) sample_total/count;
}
