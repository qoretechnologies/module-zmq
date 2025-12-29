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

static QoreStringNode* zmq_module_init();
static void zmq_module_ns_init(QoreNamespace* rns, QoreNamespace* qns);
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
#ifdef QORE_BUILD_ZMQ_DRAFT
DLLLOCAL QoreClass* initZSocketServerClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketClientClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketRadioClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZSocketDishClass(QoreNamespace& ns);
//DLLLOCAL QoreClass* initZSocketScatterClass(QoreNamespace& ns);
//DLLLOCAL QoreClass* initZSocketGatherClass(QoreNamespace& ns);
//DLLLOCAL QoreClass* initZSocketDGramClass(QoreNamespace& ns);
#endif
DLLLOCAL QoreClass* initZFrameClass(QoreNamespace& ns);
DLLLOCAL QoreClass* initZMsgClass(QoreNamespace& ns);

// qore module symbols
DLLEXPORT char qore_module_name[] = "zmq";
DLLEXPORT char qore_module_version[] = PACKAGE_VERSION;
DLLEXPORT char qore_module_description[] = "zmq module";
DLLEXPORT char qore_module_author[] = "David Nichols";
DLLEXPORT char qore_module_url[] = "http://qore.org";
DLLEXPORT int qore_module_api_major = QORE_MODULE_API_MAJOR;
DLLEXPORT int qore_module_api_minor = QORE_MODULE_API_MINOR;
DLLEXPORT qore_module_init_t qore_module_init = zmq_module_init;
DLLEXPORT qore_module_ns_init_t qore_module_ns_init = zmq_module_ns_init;
DLLEXPORT qore_module_delete_t qore_module_delete = zmq_module_delete;
DLLEXPORT qore_license_t qore_module_license = QL_MIT;
DLLEXPORT char qore_module_license_str[] = "MIT";

DLLLOCAL void init_zmq_functions(QoreNamespace& ns);
DLLLOCAL void init_zmq_constants(QoreNamespace& ns);

QoreNamespace zmqns("Qore::ZMQ");

static QoreStringNode* zmq_module_init() {
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
#ifdef QORE_BUILD_ZMQ_DRAFT
    zmqns.addSystemClass(initZSocketServerClass(zmqns));
    zmqns.addSystemClass(initZSocketClientClass(zmqns));
    zmqns.addSystemClass(initZSocketRadioClass(zmqns));
    zmqns.addSystemClass(initZSocketDishClass(zmqns));
    //zmqns.addSystemClass(initZSocketScatterClass(zmqns));
    //zmqns.addSystemClass(initZSocketGatherClass(zmqns));
    //zmqns.addSystemClass(initZSocketDGramClass(zmqns));
#endif

    init_zmq_constants(zmqns);
    init_zmq_functions(zmqns);

    return 0;
}

static void zmq_module_ns_init(QoreNamespace* rns, QoreNamespace* qns) {
    qns->addNamespace(zmqns.copy());
}

static void zmq_module_delete() {
}

// module library functions
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
