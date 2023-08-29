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
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>

//namespace Tls {
unsigned int getU32( unsigned char *p )
{
   unsigned n;

   n= (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|(p[3]);

   return n;
}

int putU32( unsigned char *p, unsigned n )
{
   p[0]= (n>>24);
   p[1]= (n>>16);
   p[2]= (n>>8);
   p[3]= (n&0xFF);

   return 4;
}

int64_t getS64( unsigned char *p )
{
    int64_t n;

    n= ((((int64_t)(p[0]))<<56) |
       (((int64_t)(p[1]))<<48) |
       (((int64_t)(p[2]))<<40) |
       (((int64_t)(p[3]))<<32) |
       (((int64_t)(p[4]))<<24) |
       (((int64_t)(p[5]))<<16) |
       (((int64_t)(p[6]))<<8) |
       (p[7]) );

   return n;
}

int putS64( unsigned char *p,  int64_t n )
{
   p[0]= (((uint64_t)n)>>56);
   p[1]= (((uint64_t)n)>>48);
   p[2]= (((uint64_t)n)>>40);
   p[3]= (((uint64_t)n)>>32);
   p[4]= (((uint64_t)n)>>24);
   p[5]= (((uint64_t)n)>>16);
   p[6]= (((uint64_t)n)>>8);
   p[7]= (((uint64_t)n)&0xFF);

   return 8;
}
//}