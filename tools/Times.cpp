/*
 * Copyright (C) 2021 Amlogic Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>
#include "Times.h"

namespace Tls {

#define USE_GETTIMEOFDAY 0

int64_t Times::getSystemTimeNs()
{
#if USE_GETTIMEOFDAY
    // Clock support varies widely across hosts. Mac OS doesn't support
    // posix clocks, older glibcs don't support CLOCK_BOOTTIME and Windows
    // is windows.
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return (t.tv_sec)*1000000000LL + (t.tv_usec)*1000LL;
#else
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[1], &t);
    int64_t mono_ns = int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
    return mono_ns;
#endif
}

int64_t Times::getSystemTimeMs()
{
#if USE_GETTIMEOFDAY
   struct timeval tv;
   long long utcCurrentTimeMillis;

   gettimeofday(&tv,0);
   utcCurrentTimeMillis= tv.tv_sec*1000LL+(tv.tv_usec/1000LL);

   return utcCurrentTimeMillis;
#else
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[1], &t);
    int64_t mono_ns = int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
    return mono_ns/1000000LL;
#endif
}

int64_t Times::getSystemTimeUs()
{
#if USE_GETTIMEOFDAY
   struct timeval tv;
   long long utcCurrentTimeMillis;

   gettimeofday(&tv,0);
   utcCurrentTimeMillis= tv.tv_sec*1000LL+(tv.tv_usec/1000LL);

   return utcCurrentTimeMillis;
#else
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[1], &t);
    int64_t mono_ns = int64_t(t.tv_sec)*1000000000LL + t.tv_nsec;
    return mono_ns/1000LL;
#endif
}
}