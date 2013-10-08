/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#pragma once

class Clockrate
{
public:
    typedef u_int64_t hertz;

private:
    int                sample_count;
    double             sample_seconds;
    std::vector<hertz> sample_buffer;
    double             sample_total;

public:
    Clockrate(int samples, double seconds) :
        sample_count(samples),
        sample_seconds(seconds),
        sample_total(0)
    {};

    void  sample(void);
    hertz rate(void);
    void  reset(void);
};
