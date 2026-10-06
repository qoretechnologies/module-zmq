# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
# Use the pinned source epoch for RPM headers and installed file timestamps.
%global source_date_epoch_from_changelog 1
%global use_source_date_epoch_as_buildtime 1
%if v"%{rpmversion}" >= v"4.20"
%global build_mtime_policy clamp_to_source_date_epoch
%else
%global clamp_mtime_to_source_date_epoch 1
%endif
%bcond_without tests
%bcond_without docs
Name: qore-zmq-module
Version: 1.2.0
Release: 5%{?dist}
Summary: ZeroMQ messaging and draft socket APIs for Qore
License: MIT AND LGPL-2.1-or-later AND MPL-2.0 AND BSD-3-Clause AND BSD-2-Clause AND Beerware
URL: https://github.com/qoretechnologies/module-zmq
Source0: %{name}-%{version}.tar.xz
Source1: libzmq-4.3.5.tar.xz
Source2: czmq-b669c7eb60cb8f587071ac6ece7cd68d30ac4017.tar.xz
Provides: bundled(zeromq) = 4.3.5
Provides: bundled(czmq) = 4.2.2~gitb669c7e
BuildRequires: cmake >= 3.21
BuildRequires: make
BuildRequires: tar
BuildRequires: xz
BuildRequires: gcc-c++
BuildRequires: pkgconfig(libsodium)
BuildRequires: pkgconfig(gnutls)
BuildRequires: pkgconfig(libbsd)
BuildRequires: pkgconfig(liblz4)
BuildRequires: pkgconfig(uuid)
BuildRequires: qore-devel >= 3.0.0~
BuildRequires: qore-rpm-macros >= 3.0.0~
%if %{with tests}
BuildRequires: python3
%endif
%if %{with docs}
BuildRequires: doxygen
%endif
%if 0%{?suse_version}
BuildRequires: util-linux
%else
BuildRequires: util-linux-core
%endif

%description
Native bindings for ZeroMQ contexts, sockets, messages, frames and CURVE
security, including CLIENT/SERVER and RADIO/DISH draft socket APIs. Includes
private draft-enabled ZeroMQ libraries and compiler metadata.

%if %{with docs}
%package doc
Summary: ZeroMQ module reference documentation and examples
BuildArch: noarch
%description doc
API reference and messaging examples for Qore's ZeroMQ module.
%endif

%prep
%autosetup
tar -xf %{SOURCE1}
tar -xf %{SOURCE2}
mv libzmq-4.3.5 libzmq
mv czmq-b669c7eb60cb8f587071ac6ece7cd68d30ac4017 czmq
# This upstream header is incorrectly marked executable; retain source mode.
chmod 644 libzmq/src/yqueue.hpp
%build
%{?set_build_flags}
. %{_rpmconfigdir}/qore/module-env.sh
qore_set_source_prefix_maps "%{qore_debug_source_dir}"
cmake -S . -B build -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE=-DNDEBUG \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DUSE_SYSTEM_ZMQ=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
  -DFETCHCONTENT_SOURCE_DIR_LIBZMQ="$PWD/libzmq" \
  -DFETCHCONTENT_SOURCE_DIR_CZMQ="$PWD/czmq" \
  -DENABLE_WS=ON -DWITH_TLS=ON -DWITH_LIBBSD=ON \
  -DCZMQ_WITH_UUID=ON -DCZMQ_WITH_LZ4=ON \
  -DCZMQ_WITH_SYSTEMD=OFF -DCZMQ_WITH_LIBCURL=OFF \
  -DCZMQ_WITH_NSS=OFF -DCZMQ_WITH_LIBMICROHTTPD=OFF \
  -DCMAKE_SKIP_RPATH=ON -DCMAKE_IGNORE_PREFIX_PATH=/usr/local \
  -DQore_DIR=%{_libdir}/cmake/Qore -DQORE_EXECUTABLE=/usr/bin/qore \
  -DQORE_QPP_EXECUTABLE=/usr/bin/qpp \
  -DQORE_GENERATE_JAVA_BINDINGS=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_Doxygen=%{!?with_docs:ON}%{?with_docs:OFF}
cmake --build build -- %{?_smp_mflags}
%if %{with docs}
printf "\nWARN_AS_ERROR = FAIL_ON_WARNINGS\n" >> build/Doxyfile
cmake --build build --target docs -- %{?_smp_mflags}
%endif
%install
DESTDIR=%{buildroot} cmake --install build
for library in libzmq czmq; do
    install -d %{buildroot}%{_licensedir}/%{name}/$library
    install -m644 $library/LICENSE $library/AUTHORS %{buildroot}%{_licensedir}/%{name}/$library/
done
# Retain notices for the small third-party sources compiled into the libraries.
sed -n '1,/\*\//p' libzmq/external/sha1/sha1.c > %{buildroot}%{_licensedir}/%{name}/libzmq/LICENSE.sha1.txt
sed -n '1,/\*\//p' czmq/src/foreign/slre/slre.h > %{buildroot}%{_licensedir}/%{name}/czmq/LICENSE.slre.txt
hardlink -t -O %{buildroot}%{_licensedir}/%{name}
chmod 755 %{buildroot}%{_libdir}/qore-modules/zmq-api-*.qmod
%if %{with docs}
install -d %{buildroot}%{_docdir}/%{name}-doc
cp -a build/docs/zmq/html %{buildroot}%{_docdir}/%{name}-doc/
install -d %{buildroot}%{_docdir}/%{name}-doc/examples/test
install -m644 test/*.qtest %{buildroot}%{_docdir}/%{name}-doc/examples/test/
hardlink -t -O %{buildroot}%{_docdir}/%{name}-doc
%endif
%check
%if %{with tests}
cmake --build build --target czmq-hash-growth-test -- %{?_smp_mflags}
build/czmq-hash-growth-test
. %{_rpmconfigdir}/qore/module-env.sh
%if %{with docs}
python3 -B -W error test/test_docs.py build -v
%endif
module="$PWD/build/zmq-api-$(/usr/bin/qore --latest-module-api).qmod"
/usr/bin/qore -b --enable-debug -l "$module" rpm/features.qr
python3 -B -W error test/run-sandbox-errors.py --module "$module"
for suite in test/*.qtest; do
    timeout 600 /usr/bin/qore -b --enable-debug -l "$module" "$suite" -v
done
%endif
%files
%license %{_licensedir}/%{name}/libzmq
%license %{_licensedir}/%{name}/czmq
%license LICENSE debian/copyright
%doc README.md
%{_libdir}/qore-modules/zmq-api-*.qmod
%dir %{_datadir}/qore/metadata/zmq
%{_datadir}/qore/metadata/zmq/*.meta.json
%if %{with docs}
%files doc
%license LICENSE debian/copyright
%doc %{_docdir}/%{name}-doc/
%endif
%changelog
* Tue Oct 06 2026 David Nichols <david@qore.org> - 1.2.0-5
- Drain monitor shutdown before releasing its receiver, including exception paths.
- Verify new socket traffic after bound, unbound and rebound fixture failures.

* Tue Oct 06 2026 David Nichols <david@qore.org> - 1.2.0-4
- Observe listener-close events before rebinding ports in regression fixtures.

* Tue Oct 06 2026 David Nichols <david@qore.org> - 1.2.0-3
- Hard-link identical vendor license payloads while retaining both notice paths.

* Tue Oct 06 2026 David Nichols <david@qore.org> - 1.2.0-2
- Consume temporary address-probe exceptions while retaining caller-visible denial.
- Verify repeated denied and allowed operations with no abandoned-error output.
- Retain required CZMQ assertion expressions and verify hash-table growth.

* Thu Oct 01 2026 David Nichols <david@qore.org> - 1.2.0-1
- Package complete draft APIs and CURVE support using pinned offline sources.
- Include metadata, reference documentation and all local messaging suites.
