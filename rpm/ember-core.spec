Name:           ember-core
Version:        2.0.3
Release:        1%{?dist}
Summary:        Ember programming language runtime

License:        MIT
URL:            https://github.com/exec/ember-core
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  readline-devel
BuildRequires:  libcurl-devel
BuildRequires:  pkgconfig

Requires:       glibc
Requires:       readline
Requires(pre):  shadow-utils
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Ember Core is the runtime environment for the Ember programming language.
It provides the core virtual machine, JIT compiler, and essential runtime
components. Features:

- High-performance virtual machine with JIT compilation
- Advanced memory management with VM pool optimization
- Lock-free concurrent execution support
- Security-hardened runtime with input validation
- NUMA-aware memory allocation
- Interactive REPL with readline support
- Comprehensive error handling and debugging support

This package contains the core Ember runtime, interpreter, and compiler
needed to run Ember programs.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

# Install core runtime files using ember-core's Makefile install target
make install PREFIX=%{buildroot}%{_prefix}

# Install headers to the proper location
install -d -m 755 %{buildroot}%{_includedir}/ember
install -D -m 644 include/ember.h %{buildroot}%{_includedir}/ember/ember.h
install -D -m 644 include/ember_interfaces.h %{buildroot}%{_includedir}/ember/ember_interfaces.h

# Install documentation
install -D -m 644 README.md %{buildroot}%{_docdir}/%{name}/README.md
install -D -m 644 CHANGELOG.md %{buildroot}%{_docdir}/%{name}/CHANGELOG.md
install -D -m 644 LICENSE %{buildroot}%{_docdir}/%{name}/LICENSE

# Create directories for ember runtime files
install -d -m 755 %{buildroot}%{_localstatedir}/lib/ember
install -d -m 755 %{buildroot}%{_localstatedir}/log/ember

%pre
getent group ember >/dev/null || groupadd -r ember
getent passwd ember >/dev/null || \
    useradd -r -g ember -d %{_localstatedir}/lib/ember -s /sbin/nologin \
    -c "Ember runtime" ember
exit 0

%post
/sbin/ldconfig
echo "Ember Core runtime installed successfully."
echo "Use 'ember' command to start the interactive REPL."
echo "Use 'emberc' command to compile Ember programs."

%postun
/sbin/ldconfig
if [ $1 -eq 0 ] ; then
    # Package removal, not upgrade
    getent passwd ember >/dev/null && userdel ember >/dev/null 2>&1 || :
    getent group ember >/dev/null && groupdel ember >/dev/null 2>&1 || :
    rm -rf %{_localstatedir}/lib/ember
    rm -rf %{_localstatedir}/log/ember
fi

%files
%license LICENSE
%doc %{_docdir}/%{name}/
%{_bindir}/ember
%{_bindir}/emberc
%{_libdir}/libember.a
%{_includedir}/ember/
%attr(755,ember,ember) %dir %{_localstatedir}/lib/ember
%attr(755,ember,ember) %dir %{_localstatedir}/log/ember

%changelog
* Sat Jun 22 2025 Ember Project <maintainer@ember-lang.org> - 2.0.3-1
- Ember Platform v2.0.3 release with major performance and security enhancements
- Advanced JIT compiler with 5-50x performance improvements
- Lock-free VM pool with work-stealing thread architecture
- NUMA-aware memory allocation and optimization
- Security-hardened runtime with comprehensive input validation
- Interactive REPL with readline support and debugging features
- Enterprise-grade virtual machine with pool optimization
- Comprehensive error handling and memory safety features
- Production-ready runtime for Ember programming language

* Mon Jun 15 2025 Ember Project <maintainer@ember-lang.org> - 2.0.2-1
- Major performance release with JIT compilation
- VM pool optimization and concurrent execution support
- Memory management enhancements
- Security hardening and input validation
- Advanced bytecode optimization
- Comprehensive test suite with security testing