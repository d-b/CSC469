/*
 * CSC469 - Performance Evaluation
 *
 * Daniel Bloemendal <d.bloemendal@gmail.com>
 */

#pragma once

typedef struct {
    u_int64_t start;
    u_int64_t end;
} period_t;

u_int64_t inactive_periods(int num, u_int64_t threshold, u_int64_t* samples);

#ifdef __cplusplus
u_int64_t inactive_periods(int num, u_int64_t threshold, std::vector<period_t>& samples);
#endif
