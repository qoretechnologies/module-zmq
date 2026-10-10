/* indent-tabs-mode: nil -*- */
/*
    Qore zmq module

    Copyright (C) 2017 - 2021 Qore Technologies, s.r.o.

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

#include "zmq-module.h"

#include <cerrno>
#include <vector>

static void zmq_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink);
static void zmq_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink);
static void zmq_module_delete();

DLLLOCAL void preinitZSocketClass();
DLLLOCAL void preinitZFrameClass();
DLLLOCAL void preinitZMsgClass();

// for hashdecls
const TypedHashDecl* hashdeclZmqVersionInfo,
    * hashdeclZmqPollInfo,
    * hashdeclZmqCurveKeyInfo;
DLLLOCAL TypedHashDecl* init_hashdecl_ZmqVersionInfo(QoreNamespace& ns);
DLLLOCAL TypedHashDecl* init_hashdecl_ZmqPollInfo(QoreNamespace& ns);
DLLLOCAL TypedHashDecl* init_hashdecl_ZmqCurveKeyInfo(QoreNamespace& ns);

DLLLOCAL QoreClass* initZContextClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketPubClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketSubClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketReqClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketRepClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketDealerClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketRouterClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketPushClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketPullClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketXPubClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketXSubClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketPairClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketStreamClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketServerClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketClientClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketRadioClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketDishClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZFrameClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZMsgClass(QoreNamespace& ns);

extern "C" DLLEXPORT void zmq_qore_module_desc(QoreModuleInfo& mod_info) {
    mod_info.name = "zmq";
    mod_info.version = PACKAGE_VERSION;
    mod_info.desc = "zmq module";
    mod_info.author = "David Nichols";
    mod_info.url = "http://qore.org";
    mod_info.api_major = QORE_MODULE_API_MAJOR;
    mod_info.api_minor = QORE_MODULE_API_MINOR;
    mod_info.init = zmq_module_init;
    mod_info.ns_init = zmq_module_ns_init;
    mod_info.del = zmq_module_delete;
    mod_info.license = QL_LGPL;
    mod_info.license_str = "LGPL-2.1-or-later";
}

DLLLOCAL void init_zmq_functions(QoreNamespace& ns);
DLLLOCAL void init_zmq_constants(QoreNamespace& ns);

QoreNamespace zmqns("Qore::ZMQ");

static void zmq_module_init(QoreModuleInitContext& ctx, ExceptionSink& xsink) {
    zmqns.addSystemClass(initZContextClass(zmqns));

    preinitZSocketClass();
    preinitZFrameClass();
    preinitZMsgClass();

    hashdeclZmqVersionInfo = init_hashdecl_ZmqVersionInfo(zmqns);
    hashdeclZmqPollInfo = init_hashdecl_ZmqPollInfo(zmqns);
    hashdeclZmqCurveKeyInfo = init_hashdecl_ZmqCurveKeyInfo(zmqns);

    zmqns.addSystemClass(initZFrameClass(zmqns));
    zmqns.addSystemClass(initZMsgClass(zmqns));

    zmqns.addSystemClass(initZSocketClass(zmqns));
    zmqns.addSystemClass(initZSocketPubClass(zmqns));
    zmqns.addSystemClass(initZSocketSubClass(zmqns));
    zmqns.addSystemClass(initZSocketReqClass(zmqns));
    zmqns.addSystemClass(initZSocketRepClass(zmqns));
    zmqns.addSystemClass(initZSocketDealerClass(zmqns));
    zmqns.addSystemClass(initZSocketRouterClass(zmqns));
    zmqns.addSystemClass(initZSocketPushClass(zmqns));
    zmqns.addSystemClass(initZSocketPullClass(zmqns));
    zmqns.addSystemClass(initZSocketXPubClass(zmqns));
    zmqns.addSystemClass(initZSocketXSubClass(zmqns));
    zmqns.addSystemClass(initZSocketPairClass(zmqns));
    zmqns.addSystemClass(initZSocketStreamClass(zmqns));
    zmqns.addSystemClass(initZSocketServerClass(zmqns));
    zmqns.addSystemClass(initZSocketClientClass(zmqns));
    zmqns.addSystemClass(initZSocketRadioClass(zmqns));
    zmqns.addSystemClass(initZSocketDishClass(zmqns));

    init_zmq_constants(zmqns);
    init_zmq_functions(zmqns);

}

static void zmq_module_ns_init(QoreNamespace* rns, QoreNamespace* qns, ExceptionSink& xsink) {
    qns->addNamespace(zmqns.copy());
}

static void zmq_module_delete() {
}

// module library functions
int qore_zmq_poll(zmq_pollitem_t* items, int nitems, int timeout_ms, const char* operation,
        ExceptionSink* xsink) {
    // the absolute deadline in microseconds (monotonic), or -1 for no timeout
    int64 deadline = timeout_ms >= 0 ? q_clock_getmicros_monotonic() + static_cast<int64>(timeout_ms) * 1000 : -1;
#ifdef _QORE_HAS_CANCELLABLE_POLL
    QoreCancelWakeupHelper cwh(xsink, operation);
    if (*xsink) {
        return QORE_ZMQ_POLL_CANCELLED;
    }
    // the items with the wakeup descriptor last, if there is one
    std::vector<zmq_pollitem_t> pitems(items, items + nitems);
    bool wakeup = cwh.fd() != QORE_CANCEL_WAKEUP_NONE;
    if (wakeup) {
        zmq_pollitem_t w;
        w.socket = nullptr;
        w.fd = cwh.fd();
        w.events = ZMQ_POLLIN;
        w.revents = 0;
        pitems.push_back(w);
    }
    while (true) {
        long wait_ms = -1;
        if (deadline >= 0) {
            int64 remaining = deadline - q_clock_getmicros_monotonic();
            wait_ms = remaining > 0 ? static_cast<long>((remaining + 999) / 1000) : 0;
        }
        int rc = zmq_poll(pitems.data(), static_cast<int>(pitems.size()), wait_ms);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (wakeup && pitems.back().revents) {
            // a request is raised here; otherwise there is nothing to deliver and the wait continues
            if (qore_cancel_wakeup_check(xsink, operation)) {
                return QORE_ZMQ_POLL_CANCELLED;
            }
            --rc;
            pitems.back().revents = 0;
        }
        if (rc > 0 || (deadline >= 0 && deadline <= q_clock_getmicros_monotonic())) {
            for (int i = 0; i < nitems; ++i) {
                items[i].revents = pitems[i].revents;
            }
            return rc;
        }
    }
#else
    // with an older Qore library without the cancellable wait API, the wait is made in slices so that a
    // cancellation request is checked regularly
    while (true) {
        if (qore_check_cancel(xsink, operation)) {
            return QORE_ZMQ_POLL_CANCELLED;
        }
        long wait_ms = QORE_IO_POLL_INTERVAL_MS;
        if (deadline >= 0) {
            int64 remaining = deadline - q_clock_getmicros_monotonic();
            if (remaining <= 0) {
                wait_ms = 0;
            } else if (remaining / 1000 < wait_ms) {
                wait_ms = static_cast<long>((remaining + 999) / 1000);
            }
        }
        int rc = zmq_poll(items, nitems, wait_ms);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (rc > 0 || (deadline >= 0 && deadline <= q_clock_getmicros_monotonic())) {
            return rc;
        }
    }
#endif
}

void zmq_error(ExceptionSink* xsink, const char* err, const char* desc_fmt, ...) {
    va_list args;

    QoreString desc;

    while (true) {
        va_start(args, desc_fmt);
        int rc = desc.vsprintf(desc_fmt, args);
        va_end(args);
        if (!rc)
            break;
    }

    desc.concat(": ");
    desc.concat(zmq_strerror(errno));

    xsink->raiseExceptionArg(errno == ETERM ? "ZSOCKET-CONTEXT-ERROR" : err, errno, desc.c_str());
}
