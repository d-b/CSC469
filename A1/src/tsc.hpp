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
    static bool fixedmode;
    bool standardmode;
    cycles initial;

public:
    TSC() : standardmode(!fixedmode) {};
    TSC(bool fixedmode) : standardmode(!fixedmode) {};

    static bool fixed();
    static void fixed(bool value);
    static cycles now();
    
    void start();
    cycles count();
};
