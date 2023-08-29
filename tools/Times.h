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
#ifndef _TOOS_TIMES_H_
#define _TOOS_TIMES_H_

#include <stdint.h>

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif
namespace Tls {
class Times {
  public:
    static int64_t getSystemTimeMs();
    static int64_t getSystemTimeUs();
    static int64_t getSystemTimeNs();
};

}

#endif /*_TOOS_TIMES_H_*/
