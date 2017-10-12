/*
 * Copyright (c) 2017 Fastly, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <inttypes.h>
#include "../../test.h"
#include "../../../../include/h2o.h"
#include "../../../../include/h2o/socket.h"

#if !H2O_USE_LIBUV
int invokes = 0;
static void my_callback(h2o_timeout_timer_t *timer)
{
    invokes++;
}

int rseed = 13;

static inline int lcg_rand()
{
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}

#define N 14000
void test_add_fixed_timers()
{
    int i;
    h2o_timeout_t *testwheel = calloc(1, sizeof(h2o_timeout_t));
    h2o_timeout_init(testwheel);

    uint64_t abs_wtime = 3;
    h2o_timeout_timer_t *timers[N];
    /* add timers */
    for (i = 0; i < N; i++) {
        uint64_t exp = abs_wtime + i + 5;
        timers[i] = h2o_timeout_create_timer(my_callback);
        h2o_timeout_add_timer_(testwheel, timers[i], h2o_timeout_abs_from_uint(exp));
    }

    /* run the wheel */
    ok(h2o_timeout__run_(testwheel, 139) == 132);
    h2o_timeout_show(testwheel);
}

void test_del_timers()
{
    int i;
    h2o_timeout_t *testwheel = calloc(1, sizeof(h2o_timeout_t));
    h2o_timeout_init(testwheel);

    uint64_t abs_wtime = 3;
    h2o_timeout_timer_t *timers[N];
    /* add N timers */
    for (i = 0; i < N; i++) {
        uint64_t exp = abs_wtime + i + 5;
        timers[i] = h2o_timeout_create_timer(my_callback);
        h2o_timeout_add_timer_(testwheel, timers[i], h2o_timeout_abs_from_uint(exp));
    }

    /* delete N-1 timers, so there should be 1 timer left */
    for (i = 0; i < N - 1; i++) {
        h2o_timeout_del_timer(timers[i]);
    }

    /* run the wheel */
    ok(h2o_timeout__run_(testwheel, N + 6) == 0);
    h2o_timeout_show(testwheel);
    ok(h2o_timeout__run_(testwheel, N + 7) == 1);
    h2o_timeout_show(testwheel);
}

void test_add_rand_timers()
{
    int i;
    h2o_timeout_t *testwheel = calloc(1, sizeof(h2o_timeout_t));
    h2o_timeout_init(testwheel);

    uint64_t abs_wtime = 3;
    h2o_timeout_timer_t *timers[N];
    /* add timers */
    for (i = 0; i < N; i++) {
        uint64_t exp = abs_wtime + lcg_rand() % N;
        timers[i] = h2o_timeout_create_timer(my_callback);
        h2o_timeout_add_timer_(testwheel, timers[i], h2o_timeout_abs_from_uint(exp));
    }

    int start = invokes;
    /* run the wheel: the timers has a max expiry N-1 + abs_wtime  */
    ok(h2o_timeout__run_(testwheel, N - 1 + abs_wtime) == N);
    ok(invokes - start == N);
    h2o_timeout_show(testwheel);
}

void test_invalid_timer()
{
    h2o_timeout_t *testwheel = calloc(1, sizeof(h2o_timeout_t));
    h2o_timeout_init(testwheel);

    h2o_timeout_timer_t timer;
    h2o_timeout_init_timer(&timer, my_callback);
    testwheel->last_run = 3;
    uint64_t exp = 0x100000007LL + testwheel->last_run;
    ok(h2o_timeout_add_timer_(testwheel, &timer, h2o_timeout_abs_from_uint(exp)) == -1);

    /* stress testing with one million outstanding timers */
#define NTIMERS 1024 * 16
    h2o_timeout_timer_t *arr = calloc(NTIMERS, sizeof(h2o_timeout_timer_t));
    exp = 11;
    int i;
    for (i = 0; i < NTIMERS; i++) {
        h2o_timeout_init_timer(&arr[i], my_callback);
        assert(h2o_timeout_add_timer_(testwheel, &arr[i], h2o_timeout_abs_from_uint(exp)) == 0);
        exp++;
    }

    exp = 11;
    for (i = 0; i < NTIMERS; i++) {
        assert(h2o_timeout__run_(testwheel, exp++) == 1);
    }
}

#endif

void test_lib__common__timeout_c()
{
#if !H2O_USE_LIBUV
    subtest("add fixed timers", test_add_fixed_timers);
    subtest("add random timers", test_add_rand_timers);
    subtest("del fixed timers", test_del_timers);
    subtest("test out-of-range timer", test_invalid_timer);
#endif
}
