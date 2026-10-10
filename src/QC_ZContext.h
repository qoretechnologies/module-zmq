/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QC_ZContext.h defines the c++ implementation of the ZContext class */
/*
  QC_ZContext.h

  Qore Programming Language

  Copyright (C) 2017 - 2018 Qore Technologies, s.r.o.

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

#ifndef _QORE_ZMQ_QC_ZCONTEXT_H

#define _QORE_ZMQ_QC_ZCONTEXT_H

#include "zmq-module.h"

#include <atomic>

class QoreZContext : public AbstractPrivateData {
public:
    // creates the object
    DLLLOCAL QoreZContext() : ctx(zmq_ctx_new()) {
    }

    DLLLOCAL void* operator*() {
        return ctx;
    }

    DLLLOCAL const void* operator*() const {
        return ctx;
    }

    //! Records the linger period of a socket of the context that is being closed
    /** A closed socket's pending messages are delivered within its linger period, which starts when it is closed;
        terminating the context waits for them

        @param linger_ms the socket's ZMQ_LINGER value in milliseconds; -1 for an infinite linger period
    */
    DLLLOCAL void socketClosed(int linger_ms);

protected:
    //! Terminates the context without waiting for the pending messages of its closed sockets
    /** see qore_zmq_terminate_context()
    */
    DLLLOCAL virtual ~QoreZContext();

private:
    void* ctx;
    //! the monotonic time in microseconds until which a closed socket can have pending messages; 0 = none
    std::atomic<int64> linger_deadline{0};
    //! the monotonic time in microseconds until which module shutdown waits for the pending messages
    std::atomic<int64> shutdown_deadline{0};

    //! Sets an atomic to a value if it is greater
    DLLLOCAL static void setMax(std::atomic<int64>& a, int64 v) {
        int64 cur = a.load();
        while (cur < v && !a.compare_exchange_weak(cur, v)) {
        }
    }
};

DLLLOCAL extern QoreClass* QC_ZCONTEXT;
DLLLOCAL extern qore_classid_t CID_ZCONTEXT;

#endif // _QORE_ZMQ_QC_ZCONTEXT_H
