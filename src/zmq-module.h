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

#ifdef QORE_BUILD_ZMQ_DRAFT
#define ZMQ_BUILD_DRAFT_API 1
#define CZMQ_BUILD_DRAFT_API 1
#endif

#include <zmq.h>

#include <stdarg.h>

DLLLOCAL void zmq_error(ExceptionSink* xsink, const char* err, const char* desc_fmt, ...);

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
