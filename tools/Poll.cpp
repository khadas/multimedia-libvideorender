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
#include <sys/time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include "Times.h"
#include "Poll.h"
#include "Logger.h"

#define TAG "Poll"

namespace Tls {

#define INCREASE_ACTIVE_FDS 8

Poll::Poll(bool controllable)
      : mControllable(controllable)
{
    mFds = NULL;
    mFdsCnt = 0;
    mFdsMaxCnt = 0;

    mWaiting.store(0);
    mControlPending.store(0);
    mFlushing.store(0);

    //init control sockets
    int control_sock[2];

    if (socketpair (PF_UNIX, SOCK_STREAM, 0, control_sock) < 0) {
        mControlReadFd = -1;
        mControlWriteFd = -1;
    } else {
        mControlReadFd = control_sock[0];
        mControlWriteFd = control_sock[1];
    }
    addFd(mControlReadFd);
    setFdReadable(mControlReadFd,true);
}

Poll::~Poll()
{
    if (mControlWriteFd >= 0) {
        close (mControlWriteFd);
        mControlWriteFd = -1;
    }

    if (mControlReadFd >= 0) {
        close (mControlReadFd);
        mControlReadFd = -1;
    }

    if (mFds) {
        free(mFds);
        mFds = NULL;
    }
}

int Poll::addFd(int fd)
{
    Tls::Mutex::Autolock _l(mMutex);
    struct pollfd *pfd = findFd(fd);
    if (pfd) {
        return 0;
    }

    if ((mFdsCnt + 1) > mFdsMaxCnt) {
        mFdsMaxCnt += INCREASE_ACTIVE_FDS;
        mFds = (struct pollfd *)realloc(mFds,mFdsMaxCnt*sizeof(struct pollfd));
        if (!mFds) {
            ERROR(NO_CAT,"NO memory");
            return -1;
        }
    }
    mFds[mFdsCnt].fd = fd;
    mFds[mFdsCnt].events = POLLERR | POLLNVAL | POLLHUP;
    mFds[mFdsCnt].revents = 0;
    mFdsCnt++;

    //DEBUG("mFds:%p,maxcnt:%d,cnt:%d",mFds,mFdsMaxCnt,mFdsCnt);
    return 0;
}

int Poll::removeFd(int fd)
{
    Tls::Mutex::Autolock _l(mMutex);
    for (int i = 0; i < mFdsCnt; i++) {
        struct pollfd *pfd = &mFds[i];
        if (pfd->fd == fd) {
            memmove(mFds+i*sizeof(struct pollfd),mFds+(i+1)*sizeof(struct pollfd), 1);
            mFdsCnt--;
            return 0;
        }
    }
    return 0;
}

int Poll::setFdReadable(int fd, bool readable)
{
    Tls::Mutex::Autolock _l(mMutex);
    struct pollfd * pfd = findFd(fd);
    if (!pfd) {
        return -1;
    }
    if (readable) {
        pfd->events |= POLLIN | POLLPRI | POLLRDNORM;
    } else {
        pfd->events &= ~POLLIN;
    }

    return 0;
}

int Poll::setFdWritable(int fd, bool writable)
{
    Tls::Mutex::Autolock _l(mMutex);
    struct pollfd * pfd = findFd(fd);
    if (!pfd) {
        return -1;
    }
    if (writable) {
        pfd->events |= POLLOUT;
    } else {
        pfd->events &= ~POLLOUT;
    }

    return 0;
}

int Poll::wait(int64_t timeoutMs /*millisecond*/)
{
    int oldwaiting;
    int activecnt = 0;

    oldwaiting = mWaiting.load();

    mWaiting.fetch_add(1);

    if (oldwaiting > 0) { //had other thread waiting
        goto tag_already_waiting;
    }

    if (mFlushing.load()) {
        goto tag_flushing;
    }

    do {
        int64_t t = -1; //nanosecond
        if (timeoutMs >= 0) {
            t = timeoutMs;
        }
        //DEBUG("waiting");
        activecnt = poll(mFds, mFdsCnt, t);
        //DEBUG("waiting end");
        if (mFlushing.load()) {
            goto tag_flushing;
        }
    } while(0);

tag_success:
    mWaiting.fetch_sub(1);
    return activecnt;
tag_already_waiting:
    mWaiting.fetch_sub(1);
    errno = EPERM;
    return -1;
tag_flushing:
    mWaiting.fetch_sub(1);
    errno = EBUSY;
    return -1;
}

void Poll::setFlushing(bool flushing)
{
    /* update the new state first */
    if (flushing) {
        mFlushing.store(1);
    } else {
        mFlushing.store(0);
    }

    if (mFlushing.load() && mControllable && mWaiting.load() > 0) {
        /* we are flushing, controllable and waiting, wake up the waiter. When we
        * stop the flushing operation we don't clear the wakeup fd here, this will
        * happen in the _wait() thread. */
        raiseWakeup();
    }
}

bool Poll::isReadable(int fd)
{
    for (int i = 0; i < mFdsCnt; i++) {
        if (mFds[i].fd == fd && ((mFds[i].revents & (POLLIN|POLLRDNORM)) != 0)) {
            return true;
        }
    }
    return false;
}

bool Poll::isWritable(int fd)
{
    for (int i = 0; i < mFdsCnt; i++) {
        if (mFds[i].fd == fd && ((mFds[i].revents & POLLOUT) != 0)) {
            return true;
        }
    }
    return false;
}

struct pollfd * Poll::findFd(int fd)
{
    for (int i = 0; i < mFdsCnt; i++) {
        struct pollfd *pfd = &mFds[i];
        if (pfd->fd == fd) {
            return pfd;
        }
    }
    return NULL;
}

bool Poll::wakeEvent()
{
    ssize_t num_written;
    while ((num_written = write (mControlWriteFd, "W", 1)) != 1) {
        if (num_written == -1 && errno != EAGAIN && errno != EINTR) {
            ERROR(NO_CAT,"failed to wake event: %s", strerror (errno));
            return false;
        }
    }
    return true;
}

bool Poll::releaseEvent()
{
    char buf[1] = { '\0' };
    ssize_t num_read;
    while ((num_read = read (mControlReadFd, buf, 1)) != 1) {
        if (num_read == -1 && errno != EAGAIN && errno != EINTR) {
            ERROR(NO_CAT,"failed to release event: %s", strerror (errno));
            return false;
        }
    }
     return true;
}
bool Poll::raiseWakeup()
{
    bool result = true;

    /* makes testing control_pending and WAKE_EVENT() atomic. */
    Tls::Mutex::Autolock _l(mMutex);
    //DEBUG("mControlPending:%d",mControlPending.load());
    if (mControlPending.load() == 0) {
        /* raise when nothing pending */
        //GST_LOG ("%p: raise", set);
        result = wakeEvent();
    }

    if (result) {
        mControlPending.fetch_add(1);
    }

     return result;
}

bool Poll::releaseWakeup()
{
    bool result = false;

    /* makes testing/modifying control_pending and RELEASE_EVENT() atomic. */
    Tls::Mutex::Autolock _l(mMutex);

    if (mControlPending.load() > 0) {
        /* release, only if this was the last pending. */
        if (mControlPending.load() == 1) {
            //GST_LOG ("%p: release", set);
            result = releaseEvent();
        } else {
            result = true;
        }

        if (result) {
            mControlPending.fetch_sub(1);
        }
    } else {
        errno = EWOULDBLOCK;
    }

    return result;
}

bool Poll::releaseAllWakeup()
{
    bool result = false;

    /* makes testing/modifying control_pending and RELEASE_EVENT() atomic. */
    Tls::Mutex::Autolock _l(mMutex);

    if (mControlPending.load() > 0) {
        //GST_LOG ("%p: release", set);
        result = releaseEvent();
        if (result) {
            mControlPending.store(0);
        }
    } else {
        errno = EWOULDBLOCK;
    }

    return result;
}

}