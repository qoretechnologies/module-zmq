/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QC_ZSocketClient.h defines the c++ implementation of the ZSocketClient class */
/*
    QC_ZSocketClient.h

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

#ifndef _QORE_ZMQ_QC_ZSOCKETCLIENT_H

#define _QORE_ZMQ_QC_ZSOCKETCLIENT_H

#include "QC_ZSocket.h"

class QoreClientZSock : public QoreZSockConnect {
public:
    // creates the object
    DLLLOCAL QoreClientZSock(QoreZContext& ctx, const char* endpoint, ExceptionSink* xsink)
            : QoreZSockConnect(ctx, ZMQ_CLIENT, endpoint, xsink) {
        // CLIENT sockets are thread-safe
        setThreadSafe();
    }

    DLLLOCAL virtual int getType() const {
        return ZMQ_CLIENT;
    }

    DLLLOCAL virtual const char* getTypeName() const {
        return "CLIENT";
    }
};

#endif // _QORE_ZMQ_QC_ZSOCKETCLIENT_H
