/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QC_ZMsg.h defines the c++ implementation of the Qore ZMsg class */
/*
    QC_ZMsg.h

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

#ifndef _QORE_ZMQ_QC_ZMSG_H

#define _QORE_ZMQ_QC_ZMSG_H

#include <qore/Qore.h>

#include <czmq.h>

class QoreZMsg : public AbstractZmqThreadLocalData {
public:
    // creates an empty msg
    DLLLOCAL QoreZMsg() : msg(zmsg_new()) {
    }

    DLLLOCAL QoreZMsg(zmsg_t* msg) : msg(msg) {
    }

    // creates msg with routing_id (for SERVER socket receives)
    DLLLOCAL QoreZMsg(zmsg_t* msg, uint32_t routing_id) : msg(msg), routing_id(routing_id) {
    }

    //! Gets the routing_id (for messages received on SERVER sockets)
    DLLLOCAL uint32_t getRoutingId() const {
        return routing_id;
    }

    //! Sets the routing_id
    DLLLOCAL void setRoutingId(uint32_t id) {
        routing_id = id;
    }

    // copies the msg
    DLLLOCAL QoreZMsg(const QoreZMsg& old) : msg(zmsg_dup(old.msg)), routing_id(old.routing_id) {
    }

    DLLLOCAL zmsg_t* operator*() {
        return msg;
    }

    DLLLOCAL const zmsg_t* operator*() const {
        return msg;
    }

    DLLLOCAL int addFrames(const QoreListNode* l, ExceptionSink* xsink);

    DLLLOCAL zmsg_t** getPtr() {
        return &msg;
    }

    //! the error string for exceptions
    DLLLOCAL virtual const char* getErrorString() const {
        return "ZMSG-THREAD-ERROR";
    }

protected:
    DLLLOCAL virtual ~QoreZMsg() {
        zmsg_destroy(&msg);
    }

    DLLLOCAL int addFrame(const QoreValue v, int i, int max, ExceptionSink* xsink);

private:
    zmsg_t* msg = nullptr;
    uint32_t routing_id = 0;  // For messages received on SERVER sockets
};

DLLLOCAL extern QoreClass* QC_ZMSG;
DLLLOCAL extern qore_classid_t CID_ZMSG;

#endif // _QORE_ZMQ_QC_ZMSG_H
