# Rai MD API for Windows (x64)

Built on Linux with the mingw-w64 cross compiler: `make port_extra=-mingw dotnet=1 dist_win`.

## What is in the zip / installer

```
bin/   raisub2.exe raipub2.exe raiping2.exe raireplay2.exe    C++ v2 programs
       raisub.exe  raipub.exe  raiping.exe                     C++ v1 programs
       draisub2.exe draipub2.exe draiping2.exe draireplay2.exe .NET programs (+ .dll/.runtimeconfig.json)
       RaiApi2.dll                                             .NET binding (netstandard2.0)
       raimdapi.dll                                            the api, C interface (raiapi2_c.h) for .NET / C
       jraiapi2.dll jraimsg.dll  jraisub2.cmd ...              java JNI dlls and launchers
       libcares-2.dll libpcre2-8-0.dll libwinpthread-1.dll zlib1.dll   mingw runtime
lib/   raimdapi.lib (import library for raimdapi.dll), libraimdapi.a (mingw static, C++ api), *.jar
include/  raiapi2.h raiapi2_c.h raiapi.h and the base/msg/stream/util headers
```

Everything is self contained: the sibling libraries (raikv, raimd, sassrv, omm, libdecnumber) are linked in statically, so no other Rai dlls are needed.

## Requirements

- Windows 10 / Server 2016 or later, x64.
- .NET programs: the .NET 9 runtime (`winget install Microsoft.DotNet.Runtime.9`), or any runtime that can roll forward from 9.0.
- Java programs: a JRE 21 on the PATH.
- A daemon to connect to: `-d <host>:7500` (the C++ and .NET programs) as on linux.

## Install

Either unzip anywhere and run from `bin\`, or run `raimdapi-<ver>-win64-setup.exe`, which installs to `%ProgramFiles%\RaiMdApi`, registers an uninstaller in Apps & features and optionally appends `bin` to the system PATH.

## Using the api from your own code

- .NET: reference `bin\RaiApi2.dll`; keep `raimdapi.dll` and the four runtime dlls next to your exe (or on PATH).  Namespaces `Com.Rai.Raiapi2`, `Com.Rai.Raimsg`.
- C (any compiler, MSVC included): `include\raiapi2_c.h` + `lib\raimdapi.lib`.
- C++: the classes in `raiapi2.h` are mingw-ABI; link `lib\libraimdapi.a` from a mingw toolchain.  For MSVC use the C interface.
- Java: `lib\*.jar` on the classpath, `-Djava.library.path=bin`.
