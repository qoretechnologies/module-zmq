/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QC_ZSocketServer.h defines the c++ implementation of the ZSocketServer class */
/*
    QC_ZSocketServer.h

    Qore Programming Language

    Copyright (C) 2017 - 2025 Qore Technologies, s.r.o.

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

#ifndef _QORE_ZMQ_QC_ZSOCKETSERVER_H

#define _QORE_ZMQ_QC_ZSOCKETSERVER_H

#include "QC_ZSocket.h"

#include <atomic>

class QoreServerZSock : public QoreZSockBind {
public:
    // creates the object
    DLLLOCAL QoreServerZSock(QoreZContext& ctx, const char* endpoint, ExceptionSink* xsink)
            : QoreZSockBind(ctx, ZMQ_SERVER, endpoint, xsink) {
        // SERVER sockets are thread-safe
        setThreadSafe();
    }

    DLLLOCAL virtual int getType() const {
        return ZMQ_SERVER;
    }

    DLLLOCAL virtual const char* getTypeName() const {
        return "SERVER";
    }

    // Get the routing_id from the last received message
    // Uses acquire ordering to ensure proper synchronization with the store
    DLLLOCAL uint32_t getRoutingId() const {
        return routing_id.load(std::memory_order_acquire);
    }

    // Set the routing_id for the next send operation
    // Uses release ordering to ensure proper synchronization with loads
    DLLLOCAL void setRoutingId(uint32_t id) {
        routing_id.store(id, std::memory_order_release);
    }

private:
    std::atomic<uint32_t> routing_id{0};
};

#endif // _QORE_ZMQ_QC_ZSOCKETSERVER_H
