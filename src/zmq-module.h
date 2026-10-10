/* indent-tabs-mode: nil -*- */
/*
    Qore zmq module

    Copyright (C) 2017 - 2020 Qore Technologies, s.r.o.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _QORE_ZMQ_MODULE_H
#define _QORE_ZMQ_MODULE_H

#include <qore/Qore.h>
#include <qore/QoreSandboxManager.h>

// Draft API is required for building the zmq module
#define ZMQ_BUILD_DRAFT_API 1
#define CZMQ_BUILD_DRAFT_API 1

#include <zmq.h>

#include <stdarg.h>

DLLLOCAL void zmq_error(ExceptionSink* xsink, const char* err, const char* desc_fmt, ...);

//! returned by qore_zmq_poll() when an exception has been raised (THREAD-CANCELLED or PROGRAM-INTERRUPTED)
#define QORE_ZMQ_POLL_CANCELLED -2

//! polls ZeroMQ items like zmq_poll(), ending the wait as soon as the thread is cancelled or its Program interrupted
/** The thread's cancellation wakeup descriptor is polled with the items, so a cancellation request ends the wait at
    once; EINTR is retried for the rest of the timeout

    @param items the items to poll
    @param nitems the number of items
    @param timeout_ms the timeout in milliseconds; negative for no timeout
    @param operation the operation named in the cancellation exception
    @param xsink for the cancellation exception

    @return the number of items with events, 0 if the timeout expired, -1 if zmq_poll() failed (errno is set), or
    @ref QORE_ZMQ_POLL_CANCELLED if an exception was raised
*/
DLLLOCAL int qore_zmq_poll(zmq_pollitem_t* items, int nitems, int timeout_ms, const char* operation,
        ExceptionSink* xsink);

//! returns the monotonic deadline in microseconds for a timeout in milliseconds, or -1 for a negative timeout
DLLLOCAL int64 qore_zmq_deadline(int timeout_ms);

//! the default timeout of sockets, in milliseconds, and the default linger period of sockets of a blocky context
#define ZSOCK_TIMEOUT_MS 120000

//! Terminates a context in the calling thread; every socket of the context must be closed
/** zmq_ctx_term() returns when the closed sockets have delivered their pending messages or their linger periods have
    expired
*/
DLLLOCAL void qore_zmq_ctx_term(void* ctx);

//! Terminates a context whose closed sockets may still have pending messages, without waiting in the calling thread
/** The context is terminated in a terminator thread, so that the caller (an object destructor) returns at once while
    pending messages are delivered within their linger periods; module shutdown waits for running terminators until
    their shutdown deadlines.  If a terminator thread cannot be started, the context is left open until the process
    exits (a warning is written to stderr): a destructor never waits for the network.

    @param ctx the context; every socket of the context must be closed
    @param shutdown_deadline the monotonic time in microseconds until which module shutdown waits for the context
*/
DLLLOCAL void qore_zmq_terminate_context(void* ctx, int64 shutdown_deadline);

//! Returns the time that module shutdown waits for a closed socket with an infinite linger period, in milliseconds
DLLLOCAL int qore_zmq_infinite_linger_cap_ms();

//! returns the milliseconds until the given deadline (0 if it has passed), or -1 if there is no deadline (-1)
DLLLOCAL int qore_zmq_remaining_ms(int64 deadline);

// for hashdecls
DLLLOCAL extern const TypedHashDecl* hashdeclZmqVersionInfo;
DLLLOCAL extern const TypedHashDecl* hashdeclZmqPollInfo;
DLLLOCAL extern const TypedHashDecl* hashdeclZmqCurveKeyInfo;

// base class for private data restricted to the thread in which it was created
class AbstractZmqThreadLocalData : public AbstractPrivateData {
public:
    DLLLOCAL int check(ExceptionSink* xsink) const {
        if (!thread_safe && tid != q_gettid()) {
            xsink->raiseException(getErrorString(), "this object was created in TID %d; it is an error to access " \
                "it from any other thread (accessed from TID %d)", tid, q_gettid());
            return -1;
        }
        return 0;
    }

    DLLLOCAL int gettid() const {
        return tid;
    }

    DLLLOCAL void setThreadSafe() {
        assert(!thread_safe);
        thread_safe = true;
    }

    //! the error string for exceptions
    virtual const char* getErrorString() const = 0;

private:
    int tid = q_gettid();
    bool thread_safe = false;
};

#endif
