RPM packaging
=============

Copyright 2026 Qore Technologies, s.r.o.

Release 4 makes the TCP rebind fixtures synchronize with ZeroMQ's
``ZMQ_EVENT_CLOSED`` notification. ``unbind()`` queues listener teardown; the
notification follows the operating-system close. Both fixtures verify the
event and endpoint before immediately rebinding the same port. No sleep,
bind retry, timeout increase or production behavior change is required.

Release 5 keeps each monitor receiver alive until ``ZMQ_EVENT_MONITOR_STOPPED``.
ZeroMQ sends monitor events synchronously; destroying the receiver first can
block subsequent listener-close events on the I/O thread. Cleanup runs on both
success and exception paths. A regression injects failures while bound, after
unbind and after rebind, then verifies fresh socket traffic on the same context.

The multi-distribution recipe targets the Qore 3 SDK on Fedora, Enterprise
Linux and openSUSE. It builds the complete CLIENT/SERVER and RADIO/DISH draft
APIs. The required draft implementations are not supplied by ordinary
system ZeroMQ libraries, so the package uses the same pinned private libzmq
4.3.5 and CZMQ revision as the Debian packaging. ``rpm/vendor-sources.json``
records upstream URLs, SHA-256 checksums, retained source paths and licenses.
Shared system sodium, GnuTLS, BSD support, LZ4 and UUID libraries are used.
Private ZeroMQ archive symbols remain hidden; no private headers or library
packages are installed. Bundled components require security monitoring.

Prepare sources outside the network-isolated build::

    python3 ../qore-packaging/tools/packaging.py prepare \
      --repo . --ref COMMIT --name qore-zmq-module --version 1.2.0 \
      --spec qore-zmq-module.spec --vendor-manifest rpm/vendor-sources.json \
      --cache ../qore-packaging/cache --output ../qore-packaging/work/zmq-source

The recipe checks draft, proxy and CURVE capabilities before running every
local messaging suite with debugging enabled. Documentation is built with
Doxygen warnings treated as errors. Runtime, metadata and native debug
information are split from the noarch documentation package. Tests and docs
are enabled by default; release qualification requires both.

To test an installed RPM without the SDK or compiler, run from an extracted
source tree as an unprivileged user::

    QORE_RPM_TEST_TMP=/tmp/zmq-installed rpm/tests-installed-runtime

Only test files are copied outside the checkout. The tests use installed
modules, with networking disabled except for the container's local loopback.

Release 1.2.0-2 consumes denied-address probe exceptions before selecting another
address or reporting the final denial. The sandbox regression repeats rejected
bind/connect operations, checks permitted operations and malformed endpoints,
and rejects unexpected standard-error output. Network policy is unchanged.

When testing alongside the optional XML module, the standalone sandbox checker
accepts ``--allow-qunit-xml-fallback``. This explicitly permits one exact QUnit
AOT source-selection warning when XML becomes available after QUnit compilation.
The warning remains visible. All other stderr and nonzero Qore exit statuses
still fail. Validate the allowance and its negative cases with::

    python3 -B -W error test/test_sandbox_error_runner.py -v

The private CZMQ archive retains assertion expressions in optimized builds:
CZMQ performs required hash-table resizing inside those expressions. The
module itself keeps the distribution's release flags. A native regression
checks 10,000 insertions, actual rehash callbacks, duplicate-key rejection,
lookups and deletion against the linked private archive.

Release 3 retains each vendor's license path while hard-linking byte-identical
license payloads, eliminating duplicate-file package lint without omitting notices.
