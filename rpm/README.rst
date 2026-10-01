RPM packaging
=============

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
