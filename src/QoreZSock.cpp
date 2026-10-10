/* -*- mode: c++; indent-tabs-mode: nil -*- */
/** @file QoreZSock.cpp defines the QoreZSock C++ class */
/*
    Qore Programming Language

    Copyright (C) 2017 - 2026 Qore Technologies, s.r.o.

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

#include "QC_ZSocket.h"

#include <regex>
#include <string>
#include <vector>

#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

// Helper function to parse port string safely
// Returns port number, or 0 for wildcard/dynamic port ("*" or "!", optionally followed by a port range)
static int parsePort(const char* port_str) {
    if (!port_str || !*port_str || *port_str == '*' || *port_str == '!') {
        return 0;  // Dynamic port
    }
    return atoi(port_str);
}

// Helper function to check TCP/UDP network access
// Returns true if access is allowed, false if denied (exception raised)
static bool checkTcpUdpAccess(QoreSandboxManager* sm, const char* hostport, int proto, bool is_bind,
                               const char* transport_name, ExceptionSink* xsink) {
    // Find the last colon (port separator)
    const char* port_sep = strrchr(hostport, ':');
    if (!port_sep) {
        xsink->raiseException("ZMQ-ENDPOINT-ERROR", "invalid %s endpoint format: missing port",
                              transport_name);
        return false;
    }

    // Extract host and port
    std::string host(hostport, port_sep - hostport);
    const char* port_str = port_sep + 1;

    // Handle IPv6 addresses in brackets: [::1]:5555
    if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
        host = host.substr(1, host.size() - 2);
    }

    // Handle bind-all addresses
    if (host == "*" || host == "0.0.0.0") {
        // For binding to all interfaces, use a generic IPv4 check
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(parsePort(port_str));
        if (is_bind) {
            return sm->network().checkBind((struct sockaddr*)&addr, sizeof(addr), proto, xsink);
        }
        return sm->checkNetworkAccess((struct sockaddr*)&addr, sizeof(addr), proto, xsink);
    }
    if (host == "::") {
        // For binding to all interfaces, use a generic IPv6 check
        struct sockaddr_in6 addr6;
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(parsePort(port_str));
        if (is_bind) {
            return sm->network().checkBind((struct sockaddr*)&addr6, sizeof(addr6), proto, xsink);
        }
        return sm->checkNetworkAccess((struct sockaddr*)&addr6, sizeof(addr6), proto, xsink);
    }

    // Enforce hostname policies before DNS resolution (connect only)
    if (!is_bind) {
        int port = parsePort(port_str);
        if (!sm->network().checkHostname(host.c_str(), port, proto)) {
            xsink->raiseException("NETWORK-ACCESS-DENIED",
                "Connection to host '%s' denied by security policy", host.c_str());
            return false;
        }
    }

    // Resolve hostname; the port is set in the addresses resolved, as the port of a ZeroMQ endpoint can be a
    // wildcard ("*" or "!", optionally with a range) that getaddrinfo() does not accept as a service
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = (proto == QSEC_NET_TCP) ? SOCK_STREAM : SOCK_DGRAM;

    int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (rc != 0) {
        xsink->raiseException("ZMQ-ENDPOINT-ERROR", "failed to resolve host '%s': %s",
                              host.c_str(), gai_strerror(rc));
        return false;
    }
    int port = parsePort(port_str);
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        if (ai->ai_family == AF_INET) {
            reinterpret_cast<struct sockaddr_in*>(ai->ai_addr)->sin_port = htons(port);
        } else if (ai->ai_family == AF_INET6) {
            reinterpret_cast<struct sockaddr_in6*>(ai->ai_addr)->sin6_port = htons(port);
        }
    }

    // Check access for resolved addresses; access is allowed if any address is allowed, otherwise the denial of the
    // first address is raised
    bool allowed = false;
    ExceptionSink denied;
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        ExceptionSink tmp;
        if (is_bind) {
            allowed = sm->network().checkBind(ai->ai_addr, ai->ai_addrlen, proto, &tmp);
        } else {
            allowed = sm->checkNetworkAccess(ai->ai_addr, ai->ai_addrlen, proto, &tmp);
        }
        if (allowed) {
            break;
        }
        // a sink that is destroyed with an exception reports it as unhandled, so each denial is kept or cleared
        if (!denied) {
            denied.assimilate(tmp);
        } else {
            tmp.clear();
        }
    }
    freeaddrinfo(res);
    if (allowed) {
        denied.clear();
    } else if (denied) {
        xsink->assimilate(denied);
    } else {
        xsink->raiseException("NETWORK-ACCESS-DENIED",
            "%s access denied by security policy", is_bind ? "bind" : "connect");
    }
    return allowed;
}

// Helper function to check network access for ZMQ endpoints
// Returns true if access is allowed, false if denied (exception raised)
static bool checkZmqNetworkAccess(const char* endpoint, bool is_bind, ExceptionSink* xsink) {
    QoreSandboxManagerHelper smh;
    if (!smh) {
        return true;  // No sandbox manager, allow all access
    }
    QoreSandboxManager* sm = smh.get();

    // Parse endpoint to determine transport type
    // ZMQ endpoints: tcp://host:port, ipc:///path, inproc://name,
    //                pgm://interface;multicast:port, epgm://interface;multicast:port
    // Note: udp:// is only supported by RADIO/DISH (draft) sockets; regular sockets reject it.
    if (strncasecmp(endpoint, "tcp://", 6) == 0) {
        return checkTcpUdpAccess(sm, endpoint + 6, QSEC_NET_TCP, is_bind, "TCP", xsink);
    }
    else if (strncasecmp(endpoint, "udp://", 6) == 0) {
        return checkTcpUdpAccess(sm, endpoint + 6, QSEC_NET_UDP, is_bind, "UDP", xsink);
    }
    else if (strncasecmp(endpoint, "ipc://", 6) == 0) {
        // IPC (Unix domain socket) - check as Unix socket
        const char* path = endpoint + 6;
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
        if (is_bind) {
            return sm->network().checkBind((struct sockaddr*)&addr, sizeof(addr), QSEC_NET_UNIX, xsink);
        }
        return sm->checkNetworkAccess((struct sockaddr*)&addr, sizeof(addr), QSEC_NET_UNIX, xsink);
    }
    else if (strncasecmp(endpoint, "inproc://", 9) == 0) {
        // In-process transport - no network access needed
        return true;
    }
    else if (strncasecmp(endpoint, "pgm://", 6) == 0 || strncasecmp(endpoint, "epgm://", 7) == 0) {
        // PGM/EPGM multicast format: [e]pgm://interface;multicast_address:port
        // We check the multicast address, not the interface
        const char* addr_start = strstr(endpoint, "://") + 3;

        // Find interface separator (semicolon)
        const char* semicolon = strchr(addr_start, ';');
        const char* multicast_start;
        if (semicolon) {
            // Format: interface;multicast:port - use the multicast address
            multicast_start = semicolon + 1;
        } else {
            // Format: multicast:port (no interface specified)
            multicast_start = addr_start;
        }

        return checkTcpUdpAccess(sm, multicast_start, QSEC_NET_UDP, is_bind, "PGM", xsink);
    }
    else if (strncasecmp(endpoint, "vmci://", 7) == 0) {
        // VMware virtual socket - no standard network check; deny in sandboxed mode
        xsink->raiseException("NETWORK-ACCESS-DENIED",
            "vmci:// transport denied by security policy");
        return false;
    }

    // Unknown transport - deny by default in sandboxed mode (ZMQ may support new transports)
    xsink->raiseException("NETWORK-ACCESS-DENIED",
        "Unknown transport denied by security policy");
    return false;
}

static std::regex url_port_regex("^tcp://.*:(\\d+|\\*)$", std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);

int QoreZSock::poll(short events, int timeout_ms, const char* meth, ExceptionSink *xsink) {
    // Check for interrupt before poll
    if (qore_check_cancel(xsink))
        return -1;

    zmq_pollitem_t p = { sock, 0, events, 0 };
    // the wait ends at once when the thread is cancelled or its Program is interrupted
    int rc = qore_zmq_poll(&p, 1, timeout_ms, meth, xsink);
    if (rc == QORE_ZMQ_POLL_CANCELLED) {
        return -1;
    }
    if (rc < 0) {
        zmq_error(xsink, "ZSOCKET-TIMEOUT", "error in zmq_poll() in %s()", meth);
        return -1;
    }
    if (!rc) {
        xsink->raiseException("ZSOCKET-TIMEOUT", "timeout waiting %d ms in %s() for data%s on the socket",
            timeout_ms, meth, events & ZMQ_POLLOUT ? " to be sent" : "");
        return -1;
    }
    return 0;
}

// like czmq's zsock_attach()
int QoreZSock::attach(ExceptionSink *xsink, const char* endpoints, bool do_bind) {
    assert(endpoints);
    // get a vector of all endpoints with regex_token_iterator
    std::regex re(",");
    // passing -1 as the submatch index parameter performs splitting
    std::cregex_token_iterator first{endpoints, endpoints + strlen(endpoints), re, -1}, last;
    std::vector<std::string> vec = {first, last};

    // iterate all endpoints
    for (auto& str : vec) {
        if (str[0] == '@') {
            if (bind(xsink, &str.c_str()[1]) == -1)
                return -1;
        }
        else if (str[0] == '>') {
            if (connect(xsink, &str.c_str()[1]))
                return -1;
        }
        else if (do_bind) {
            if (bind(xsink, str.c_str()) == -1)
                return -1;
        }
        else if (connect(xsink, str.c_str()))
            return -1;
    }

    return 0;
}

int QoreZSock::bind(ExceptionSink *xsink, const char* endpoint, const char* err) {
    // Check for interrupt before bind
    if (qore_check_cancel(xsink))
        return -1;

    // Check network access for sandbox
    if (!checkZmqNetworkAccess(endpoint, true, xsink))
        return -1;

    std::cmatch match;
    if (regex_search(endpoint, match, url_port_regex)) {
        assert(match.ready());
        assert(match.size() == 2);
        if (!zmq_bind(sock, endpoint)) {
            // get port specification
            std::string str = match[1];
            int port = atoi(str.c_str());
            if (!port) {
                // get actual port bound
                char le[1024];
                size_t size = sizeof(le);
                if (!getSocketOption(ZMQ_LAST_ENDPOINT, &le, &size)) {
                    const char* p = strrchr(le, ':');
                    if (p)
                        port = atoi(p + 1);
                }
            }
            return port;
        }
    }
    else if (!zmq_bind(sock, endpoint)) {
        // NOTE: zmq_bind() is not affected by EINTR
        return 0;
    }

    zmq_error(xsink, err, "failed to bind to \"%s\"", endpoint);
    return -1;
}

int QoreZSock::connect(ExceptionSink *xsink, const char* endpoint, const char* err) {
    // Check for interrupt before connect
    if (qore_check_cancel(xsink))
        return -1;

    // Check network access for sandbox
    if (!checkZmqNetworkAccess(endpoint, false, xsink))
        return -1;

    // NOTE: zmq_connect() is not affected by EINTR
    int rc = zmq_connect(sock, endpoint);
    if (rc)
        zmq_error(xsink, err, "failed to connect to \"%s\"", endpoint);
    return rc;
}
