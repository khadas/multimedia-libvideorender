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
#ifndef _TOOS_THREAD_H_
#define _TOOS_THREAD_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

typedef void* (*pthread_entry_func)(void*);
#define MAX_THREAD_NAME_LEN 128
namespace Tls {
class Thread
{
  public:
    explicit Thread();
    virtual ~Thread();
    // Start the thread in threadLoop() which needs to be implemented.
    virtual int run(const char *name);

    // Good place to do one-time initializations
    virtual void readyToRun();
    // Good place to do one-time exit
    virtual void readyToExit();

    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void requestExit();
    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
    int    requestExitAndWait();
    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
    int join();
    // Indicates whether this thread is running or not.
    bool isRunning() const;

    //set thread priority from 1 ~ 99, this will set
    //thread policy to SCHED_RR,this api must be called before
    //calling run api
    void setThreadPriority(int priority);

  protected:
    // isExitPending() returns true if requestExit() has been called.
    bool isExitPending() const;
  private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool threadLoop() = 0;
private:
    Thread& operator=(const Thread&);
    static  void* _threadLoop(void* user);
    int _createThread(pthread_entry_func entryFunction);
    // always hold mLock when reading or writing
    pthread_t mThread;
    char    mThreadName[MAX_THREAD_NAME_LEN];
    mutable std::mutex mMutex;
    std::condition_variable mCond;
    int        mStatus;
    // note that all accesses of mExitPending and mRunning need to hold mLock
    volatile bool mExitPending;
    volatile bool mRunning;
    int mPriority;
};
}




#endif /*_TOOS_THREAD_H_*/