/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#pragma once

class TSC {
public:
    typedef u_int64_t cycles;

private:
    cycles initial;

public:
    void start();
    cycles count();
};