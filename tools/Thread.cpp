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
#include <assert.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <string.h>
#include <errno.h>
#include "Thread.h"
namespace Tls {
pthread_t getThreadId()
{
    return (pthread_t)pthread_self();
}

Thread::Thread()
    :mThread(pthread_t(-1)),
    mLock("Thread::mLock"),
    mStatus(0),
    mExitPending(false),
    mRunning(false),
    mPriority(0)
{
}

Thread::~Thread()
{
}

void Thread::setThreadPriority(int priority)
{
    mPriority = priority;
}

void Thread::readyToRun()
{

}

void Thread::readyToExit()
{

}

int Thread::run(const char* name)
{
    Tls::Mutex::Autolock _l(mLock);
    if (mRunning) {
        // thread already started
        return -1;
    }

    // reset status and exitPending to their default value, so we can
    // try again after an error happened (either below, or in readyToRun())
    mStatus = 0;
    mExitPending = false;
    mThread = pthread_t(-1);
    memset(mThreadName, '\0', sizeof(mThreadName));
    if (name) {
        if (strlen(name) <= sizeof(mThreadName)-1) {
            strcpy(mThreadName, name);
        } else {
            strncpy(mThreadName, name, sizeof(mThreadName)-1);
        }
    } else {
        strcpy(mThreadName, "unknown");
    }

    //set running at this ,if request exit immediately when invoking run
    //this will let requestExitAndWait block until thread running
    mRunning = true;

    int res;
    res = _createThread(_threadLoop);
    if (res != 0) {
        mStatus = -1;   // something happened!
        mRunning = false;
        mThread = pthread_t(-1);
        mCondition.broadcast();
        return -1;
    }

    return 0;
}

int Thread::_createThread(pthread_entry_func entryFunction)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    errno = 0;
    int result = pthread_create(&mThread, &attr,
                    (pthread_entry_func)entryFunction, (void *)this);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        return -1;
    }

    return 0;
}

void* Thread::_threadLoop(void* user)
{
    Thread* const self = static_cast<Thread*>(user);
    struct sched_param schedParam;
    int maxPriority,miniPriority;

    self->mRunning = true;
    if (self->mPriority > 0) {
        maxPriority = sched_get_priority_max(SCHED_FIFO);
        miniPriority = sched_get_priority_min(SCHED_FIFO);
        schedParam.sched_priority = self->mPriority;
        if (self->mPriority > maxPriority) {
            schedParam.sched_priority = maxPriority;
        } else if (self->mPriority < miniPriority) {
            schedParam.sched_priority = miniPriority;
        }
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam );
    }
    pthread_setname_np(pthread_self(), self->mThreadName);

    self->readyToRun();

    //maybe readyToRun will do something that take a long time,
    //so we need to check exit thread status
    if (self->mExitPending) {
        goto exit;
    }

    do {
        bool result = true;

        result = self->threadLoop();
        if (result == false || self->mExitPending) {
            break;
        }
    } while(self->mRunning);

exit:
    //must call readyToExit before set mRunning to false
    self->readyToExit();
    Tls::Mutex::Autolock _l(self->mLock);
    self->mExitPending = true;
    // clear thread ID so that requestExitAndWait() does not exit if
    // called by a new thread using the same thread ID as this one.
    self->mThread = pthread_t(-1);
    self->mRunning = false;
    // note that interested observers blocked in requestExitAndWait are
    // awoken by broadcast, but blocked on mLock until break exits scope
    self->mCondition.broadcast();

    return 0;
}

void Thread::requestExit()
{
    Tls::Mutex::Autolock _l(mLock);
    mExitPending = true;
}

int Thread::requestExitAndWait()
{
    Tls::Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {

        return -1;
    }

    mExitPending = true;

    while (mRunning == true) {
        mCondition.wait(mLock);
    }

    mExitPending = false;

    return mStatus;
}

int Thread::join()
{
    Tls::Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {

        return -1;
    }

    while (mRunning == true) {
        mCondition.wait(mLock);
    }

    return mStatus;
}

bool Thread::isRunning() const {
    Tls::Mutex::Autolock _l(mLock);
    return mRunning;
}

bool Thread::isExitPending() const
{
    Tls::Mutex::Autolock _l(mLock);
    return mExitPending;
}
}