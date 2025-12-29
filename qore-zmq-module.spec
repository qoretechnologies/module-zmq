%{?_datarootdir: %global mydatarootdir %_datarootdir}
%{!?_datarootdir: %global mydatarootdir %{buildroot}/usr/share}

%global module_api %(qore --latest-module-api 2>/dev/null)
%global module_dir %{_libdir}/qore-modules
%global user_module_dir %{mydatarootdir}/qore-modules/

Name:           qore-zmq-module
Version:        1.1.0
Release:        1
Summary:        Qorus Integration Engine - Qore zmq module
License:        MIT
Group:          Development/Languages/Other
Url:            https://qoretechnologies.com
Source:         qore-zmq-module-%{version}.tar.bz2
BuildRequires:  gcc-c++
%if 0%{?el7}
BuildRequires:  devtoolset-7-gcc-c++
%endif
BuildRequires:  cmake >= 3.5
BuildRequires:  qore >= 1.12.4
BuildRequires:  qore-devel >= 1.12.4
BuildRequires:  qore-stdlib >= 1.12.4
BuildRequires:  doxygen
Requires:       qore-module(abi)%{?_isa} = %{module_api}
Requires:       %{_bindir}/env
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  czmq-devel
Requires:       libzmq5 >= 4.2.3

%description
This package contains the zmq module for an ZeroMQ API for the Qore Programming Language.

%prep
%setup -q

%build
%if 0%{?el7}
# enable devtoolset7
. /opt/rh/devtoolset-7/enable
%endif
export CXXFLAGS="%{?optflags}"
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DCMAKE_SKIP_RPATH=1 -DCMAKE_SKIP_INSTALL_RPATH=1 -DCMAKE_SKIP_BUILD_RPATH=1 -DCMAKE_PREFIX_PATH=${_prefix}/lib64/cmake/Qore .
make %{?_smp_mflags}
make %{?_smp_mflags} docs
sed -i 's/#!\/usr\/bin\/env qore/#!\/usr\/bin\/qore/' test/*.qtest

%install
make DESTDIR=%{buildroot} install %{?_smp_mflags}

%files
%{module_dir}

%check
qore -l ./zmq-api-%{module_api}.qmod test/zmq.qtest -v

%package doc
Summary: Documentation and examples for the Qore zmq module
Group: Development/Languages/Other

%description doc
This package contains the HTML documentation and example programs for the Qore
zmq module.

%files doc
%defattr(-,root,root,-)
%doc docs/zmq test

%changelog
* Sun Dec 29 2024 David Nichols <david@qore.org>
- updated to v1.1.0
- added ZFrame::readi2N(), readi4N(), readi8N() for network byte order reading
- added ZSocket::tryRecvMsg(), tryRecvFrame() for non-blocking receive
- added ZSocket::hasMore() to check for additional message parts
- added ZSocket::setRecvHighWaterMark(), setSendHighWaterMark()
- added ZSocket::proxySteerable() for steerable proxy support
- added zmq_z85_decode() function
- added HAVE_ZMQ_PROXY_STEERABLE constant
- fixed exception name in ZFrame::readi4()
- fixed boundary checks in ZFrame read methods
- fixed string option handling in ZSocket::getOption()

* Tue Dec 20 2022 David Nichols <david@qore.org>
- updated to v1.0.2

* Tue Feb 15 2022 David Nichols <david@qore.org>
- updated to v1.0.1

* Thu Jan 27 2022 David Nichols <david@qore.org>
- initial 1.0.0 release

