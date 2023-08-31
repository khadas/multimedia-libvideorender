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
#ifndef _TOOS_UTILS_H_
#define _TOOS_UTILS_H_

/**
 * ROUND_UP_2:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 2.
 */
#define ROUND_UP_2(num)  (((num)+1)&~1)
/**
 * ROUND_UP_4:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 4.
 */
#define ROUND_UP_4(num)  (((num)+3)&~3)
/**
 * ROUND_UP_8:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 8.
 */
#define ROUND_UP_8(num)  (((num)+7)&~7)
/**
 * ROUND_UP_16:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 16.
 */
#define ROUND_UP_16(num) (((num)+15)&~15)
/**
 * ROUND_UP_32:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 32.
 */
#define ROUND_UP_32(num) (((num)+31)&~31)
/**
 * ROUND_UP_64:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 64.
 */
#define ROUND_UP_64(num) (((num)+63)&~63)
/**
 * ROUND_UP_128:
 * @num: integer value to round up
 *
 * Rounds an integer value up to the next multiple of 128.
 * Since: 1.4
 */
#define ROUND_UP_128(num) (((num)+127)&~127)
/**
 * ROUND_UP_N:
 * @num: integrer value to round up
 * @align: a power of two to round up to
 *
 * Rounds an integer value up to the next multiple of @align. @align MUST be a
 * power of two.
 */
#define ROUND_UP_N(num,align) ((((num) + ((align) - 1)) & ~((align) - 1)))


/**
 * ROUND_DOWN_2:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 2.
 */
#define ROUND_DOWN_2(num)  ((num)&(~1))
/**
 * ROUND_DOWN_4:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 4.
 */
#define ROUND_DOWN_4(num)  ((num)&(~3))
/**
 * ROUND_DOWN_8:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 8.
 */
#define ROUND_DOWN_8(num)  ((num)&(~7))
/**
 * ROUND_DOWN_16:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 16.
 */
#define ROUND_DOWN_16(num) ((num)&(~15))
/**
 * ROUND_DOWN_32:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 32.
 */
#define ROUND_DOWN_32(num) ((num)&(~31))
/**
 * ROUND_DOWN_64:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 64.
 */
#define ROUND_DOWN_64(num) ((num)&(~63))
/**
 * ROUND_DOWN_128:
 * @num: integer value to round down
 *
 * Rounds an integer value down to the next multiple of 128.
 * Since: 1.4
 */
#define ROUND_DOWN_128(num) ((num)&(~127))
/**
 * ROUND_DOWN_N:
 * @num: integrer value to round down
 * @align: a power of two to round down to
 *
 * Rounds an integer value down to the next multiple of @align. @align MUST be a
 * power of two.
 */
#define ROUND_DOWN_N(num,align) (((num) & ~((align) - 1)))

/**
 * @brief get unsigned int value from char p buffer
 *
 * @param p char buffer
 * @return unsigned int
 */
unsigned int getU32( unsigned char *p );

/**
 * @brief put unsigned int n to char *p
 *
 * @param p the char buffer
 * @param n unsigned int value been put
 * @return int return 4 bytes that is sizeof(int)
 */
int putU32( unsigned char *p, unsigned n );

/**
 * @brief get signed int 64 bit value from char p buffer
 *
 * @param p char buffer
 * @return signed int 64 bit value
 */
int64_t getS64( unsigned char *p );

/**
 * @brief put int 64 bit value of n to char *p
 *
 * @param p the char buffer
 * @param n int 64 bit value
 * @return int 8 bytes that is sizeof(int64_t)
 */
int putS64( unsigned char *p,  int64_t n );

#define TIME_SECOND (1000000000)
#define TIME_INVALID_VALUE             ((int64_t) -1)

#define TIME_IS_VALID(time)   (((int64_t)(time)) != TIME_INVALID_VALUE)
/* timestamp debugging macros */
/**
 * TIME_FORMAT: (skip):
 *
 * A string that can be used in printf-like format strings to display a
 * pts value in h:m:s format.  Use TIME_ARGS() to construct
 * the matching arguments.
 *
 * Example:
 * printf(GST_TIME_FORMAT "\n", TIME_ARGS(ts));
 * ]|
 */
#define TIME_FORMAT "%u:%02u:%02u.%09u"
/**
 * TIME_ARGS: (skip):
 *
 * Format pts for the #TIME_FORMAT format string. Note: pts will be
 * evaluated more than once.
 */
#define TIME_ARGS(t) \
        TIME_IS_VALID (t) ? \
        (uint32_t) (((int64_t)(t)) / (TIME_SECOND * 60 * 60)) : 99, \
        TIME_IS_VALID (t) ? \
        (uint32_t) ((((int64_t)(t)) / (TIME_SECOND * 60)) % 60) : 99, \
        TIME_IS_VALID (t) ? \
        (uint32_t) ((((int64_t)(t)) / TIME_SECOND) % 60) : 99, \
        TIME_IS_VALID (t) ? \
        (uint32_t) (((int64_t)(t)) % TIME_SECOND) : 999999999
#endif /*_TOOS_UTILS_H_*/