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
#define LOG_NDEBUG 0
#define LOG_TAG  "videorender"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <cutils/log.h>
#include "Logger.h"

#define MAX_USER_TAG 8
#define MAX_TAG_LENGTH 64
#define MAX_FILENAME_LENGTH 128
#define MAX_LOG_BUFFER 1024

static long long getCurrentTimeMillis(void);
typedef struct {
    char tag[MAX_TAG_LENGTH]; //user print tag
    bool active;
} UserTag;

static int g_activeLevel= 2;
static FILE * g_fd = stderr;
static char g_fileName[MAX_FILENAME_LENGTH];
static UserTag g_userTag[MAX_USER_TAG];
static int g_activeUserTag = 0;
static std::mutex g_mutext;
static int g_init = 0;

int Logger_init(int id)
{
    int category = NO_CAT;

    g_mutext.lock();
    if (g_init == 0) {
        g_init = 1;
        memset(g_fileName, 0 , MAX_FILENAME_LENGTH);
        for (int i = 0; i < MAX_USER_TAG; i++) {
            g_userTag[i].active = false;
        }
    }
    for (int i = 0; i < MAX_USER_TAG; i++) {
        if (g_userTag[i].active == false) {
            g_userTag[i].active = true;
            category = i;
            memset(g_userTag[i].tag, 0, MAX_TAG_LENGTH);
            sprintf(g_userTag[i].tag,"%s-%d","rlib",id);
            ++g_activeUserTag;
            break;
        }
    }
    g_mutext.unlock();
    return category;
}

void Logger_exit(int category)
{
    g_mutext.lock();
    if (category >= 0 && category < MAX_USER_TAG) {
        g_userTag[category].active = false;
        --g_activeUserTag;
    }
    g_mutext.unlock();
}

void Logger_set_level(int setLevel)
{
    if (setLevel <=0) {
        g_activeLevel = 0;
    } else if (setLevel > 6) {
        g_activeLevel = 6;
    } else {
        g_activeLevel = setLevel;
    }
}

int Logger_get_level()
{
    return g_activeLevel;
}

void Logger_set_file(char *filepath)
{
    FILE * logFd;
    if (!filepath) {
        if (g_fd != stderr) {
            fclose(g_fd);
            g_fd = stderr;
            memset(g_fileName, 0 , MAX_FILENAME_LENGTH);
        }
        return;
    } else { //close pre logfile
        //had set filepath
        if (strcmp(g_fileName, filepath) == 0) {
            return;
        }
        if (g_fd != stderr && strlen(g_fileName) > 0) {
            fprintf(stderr, "libvideorender log file:%s \n",g_fileName);
            return;
        }
        fprintf(stderr, "libvideorender log file:%s \n",g_fileName);
        memset(g_fileName, 0 , MAX_FILENAME_LENGTH);
        strcpy(g_fileName, filepath);
        if (g_fd != stderr) {
            fclose(g_fd);
            g_fd = stderr;
        }
    }

    logFd = fopen(filepath, "w");
    if (logFd == NULL) {
        return;
    }
    g_fd = logFd;
}

char *logLevelToString(int level) {
    if (level == LOG_LEVEL_ERROR) {
        return (char *)" E ";
    } else if (level == LOG_LEVEL_WARNING) {
        return (char *)" W ";
    }  else if (level == LOG_LEVEL_INFO) {
        return (char *)" I ";
    }  else if (level == LOG_LEVEL_DEBUG) {
        return (char *)" D ";
    }  else if (level == LOG_LEVEL_TRACE) {
        return (char *)" V ";
    }
    return (char *) " U ";
}

void logPrint(int category ,int level, const char *fmt, ... )
{
    if ( level <= g_activeLevel )
    {
        if (g_fd == stderr) { //default output log to logcat
            va_list argptr;
            char buf[MAX_LOG_BUFFER];
            int len = 0;

            len = sprintf(buf, "%lld ",getCurrentTimeMillis());
            if (g_activeUserTag && category >= 0 && category < MAX_USER_TAG && g_userTag[category].active) {
                int tlen = len > 0? len:0;
                tlen = sprintf( buf+tlen, "%s ", g_userTag[category].tag);
                if (tlen >= 0) {
                    len += tlen;
                }
            }
            va_start( argptr, fmt );
            if (len > 0) {
                vsnprintf(buf+len, MAX_LOG_BUFFER-len, fmt, argptr);
            } else {
                vsnprintf(buf, MAX_LOG_BUFFER, fmt, argptr);
            }
            va_end( argptr );
            ALOGI("%s", buf);
        } else if (g_fd != stderr) { //set output log to file
            va_list argptr;
            fprintf( g_fd, "%lld ", getCurrentTimeMillis());
            if (g_activeUserTag && category >= 0 && category < MAX_USER_TAG && g_userTag[category].active) {
                fprintf( g_fd, "%s ", g_userTag[category].tag);
            } else {
                fprintf( g_fd, "%d:%lu ", getpid(),pthread_self());
            }
            //print log level tag
            fprintf( g_fd, "%s ",logLevelToString(level));
            va_start( argptr, fmt );
            vfprintf( g_fd, fmt, argptr );
            va_end( argptr );
            fflush(g_fd);
        }
    }
}

static long long getCurrentTimeMillis(void)
{
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
}