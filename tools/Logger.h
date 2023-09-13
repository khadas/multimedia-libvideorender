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
#ifndef __TOOLS_LOGGER_H__
#define __TOOLS_LOGGER_H__
#include <stdarg.h>

#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_TRACE   4

#define NO_CAT -1

/**
 * @brief init logger print
 * @param id plugin instance id
 *
 * @return category id
 */
int Logger_init(int id);

/**
 * @brief exit logger print
 * @param category category id
 *
 * @return
 */
void Logger_exit(int category);

void logPrint(int category, int level, const char *fmt, ... );

/**
 * @brief set log level
 * the log level from 0 ~ 4
 * @param setLevel the log level
 * 0 - ERROR
 * 1.- WARN
 * 2.- INFO
 * 3.- DEBUG
 * 4.- TRACE
 */
void Logger_set_level(int setLevel);

/**
 * @brief get loger level
 *
 * @return int the log level from 0~6
 */
int Logger_get_level();

/**
 * @brief set log file,the filepath must is a absolute path,
 * if set filepath null, will close file and print log to stderr
 * the stderr is a default print out file
 * @param filepath the file absolute path
 */
void Logger_set_file(char *filepath);

#define INT_ERROR(CAT,FORMAT, ...)      logPrint(CAT,LOG_LEVEL_ERROR,  "%s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_WARNING(CAT,FORMAT, ...)    logPrint(CAT,LOG_LEVEL_WARNING,"%s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_INFO(CAT,FORMAT, ...)       logPrint(CAT,LOG_LEVEL_INFO,   "%s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_DEBUG(CAT,FORMAT, ...)      logPrint(CAT,LOG_LEVEL_DEBUG,  "%s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)
#define INT_TRACE(CAT,FORMAT, ...)      logPrint(CAT,LOG_LEVEL_TRACE, "%s,%s:%d " FORMAT "\n",TAG,__func__, __LINE__, __VA_ARGS__)

#define ERROR(CAT,...)                  INT_ERROR(CAT,__VA_ARGS__, "")
#define WARNING(CAT,...)                INT_WARNING(CAT,__VA_ARGS__, "")
#define INFO(CAT,...)                   INT_INFO(CAT,__VA_ARGS__, "")
#define DEBUG(CAT,...)                  INT_DEBUG(CAT,__VA_ARGS__, "")
#define TRACE(CAT,...)                  INT_TRACE(CAT,__VA_ARGS__, "")

#endif /*__TOOLS_LOGGER_H__*/