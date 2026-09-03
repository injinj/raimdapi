Name:		raiapi
Version:	999.999
Vendor:	        Rai Technology, Inc
Release:	99999%{?dist}
Summary:	Rai market data api

License:	ASL 2.0
URL:		https://github.com/raitechnology/%{name}
Source0:	%{name}-%{version}-99999.tar.gz
BuildRoot:	${_tmppath}
Prefix:	        /usr

# --- Java bindings (JNI + jars) ---------------------------------------------
# One JDK version per chroot, threaded through to javac as --release so the
# emitted class files always load on that chroot's runtime.
#   EL7:      only java-1.8.0 / java-11 exist
#   EL8+/Fed: java-21 (LTS; EL10's primary JDK)
# Build with --without java to skip the bindings entirely (C++ libs only).
# (NB: rpm expands macros inside comments -- write %%macro in comments.)
%bcond_without java
%if 0%{?rhel} == 7
%global jdk_ver 11
%else
%global jdk_ver 21
%endif

# --- .NET bindings ----------------------------------------------------------
# RaiApi2.dll (netstandard2.0, P/Invoke over libraiapi2c.so) plus the
# d{raisub2,raipub2,raiping2,raireplay2} programs.  Needs a dotnet SDK in the
# chroot (dotnet-sdk-9.0 on EL8/9/10 + Fedora; nothing on EL7), so it is off by
# default; build --with dotnet to enable.
%bcond_with dotnet

BuildRequires:  gcc-c++
BuildRequires:  chrpath
BuildRequires:  git-core
%if %{with java}
BuildRequires:  java-%{jdk_ver}-openjdk-devel
%endif
%if %{with dotnet}
BuildRequires:  dotnet-sdk-9.0
%endif
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
Market data api

%prep
%setup -q


%define _unpackaged_files_terminate_build 0
%define _missing_doc_files_terminate_build 0
%define _missing_build_ids_terminate_build 0
%define _include_gdb_index 1

%build
make build_dir=./usr %{?_smp_mflags} \
     java=%{with java} jdk_release=%{jdk_ver} dotnet=%{with dotnet} \
     dist_bins
mkdir -p ./usr/share/doc/%{name}
cp -a ./include ./usr/include
cp -a README.md doc/msg_format.md ./usr/share/doc/%{name}/

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

# in builddir
cp -a * %{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/bin/*
/usr/lib64/*
/usr/include/*
/usr/share/doc/*

%post
echo "${RPM_INSTALL_PREFIX}/lib64" > /etc/ld.so.conf.d/%{name}.conf
/sbin/ldconfig

%postun
if [ $1 -eq 0 ] ; then
rm -f /etc/ld.so.conf.d/%{name}.conf
fi
/sbin/ldconfig

%changelog
* Sat Jan 01 2000 <support@raitechnology.com>
- Hello world
