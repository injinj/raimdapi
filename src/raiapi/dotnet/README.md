# Rai API .NET binding

`RaiApi2.dll` is a .NET Standard 2.0 class library that mirrors the Java
binding in `../java` one to one, so the Java documentation and programs apply.
It loads in .NET Framework 4.7.2 / 4.8 and in .NET 6 / 8 / 9.

| Java package         | C# namespace         | classes                                              |
|----------------------|----------------------|------------------------------------------------------|
| `com.rai.raiapi2`    | `Com.Rai.Raiapi2`    | `RaiApi`, `RaiSession`, `RaiQueue`, `RaiSubscribe`, `RaiPublish`, `RaiInteractivePublish`, `RaiTimer`, `RaiDict`, `RaiEntitlement`, `Args`, `StringArg`/`BoolArg`/`IntArg`/`DoubleArg`, `Time`, `TimeRotate`, the `Rai*Event` classes and the `Rai*Callback` interfaces |
| `com.rai.raimsg`     | `Com.Rai.Raimsg`     | `RaiMsg`, `RaiField`, `Partial`, `SassConst`, `RaiMsgException` |
| `com.rai.raiexception` | `Com.Rai.Raiexception` | `RaiException` (`getModule()`, `getErrno()`, `getReason()`) |

Method names are the Java ones (`CreateSession`, `getString`, `AppendUShort`,
...).  Differences forced by the platform:

- `RaiApi.RegisterSigHandler( Action<int> )` takes a delegate instead of a
  class/method name.
- `RaiMsg.Print` / `PrintHex` / `PrintXML` take a `TextWriter` or `Stream`.
- `RaiMsg` and `RaiField` are `IDisposable`; a message handed to `onMsg()` is
  owned by the api and only valid during the callback (as in Java).
- `Time.currentTimeMillis()` replaces `System.currentTimeMillis()`.

## How it works

```
 C#  RaiApi2.dll  --P/Invoke-->  libraiapi2c.so / .dll  --C++-->  libraiapi2, libraimsg, ...
```

`libraiapi2c` (`include/raiapi2_c.h`, `src/raiapi/c/raiapi2_c.cpp`) is a flat
C API over the C++ classes: opaque handles, every call returns a `rai_err_t`
(NULL = ok), callbacks are function pointers with a closure.  It is what any
language without C++ access should bind to.  The native library is resolved by
the runtime's normal probing: put `libraiapi2c.so` (and the libraries it links,
`libraiapi2`, `libraimsg`, `libomm`, `librv7lib`, `libsassrv`, `libraimd`,
`libdecnumber`, `libraikv`, plus c-ares / pcre2 / zlib) on `LD_LIBRARY_PATH`
or next to the application, or install the rpm.  On Windows the same set as
`.dll` files (built with `make port_extra=-mingw`) goes next to the exe.

## Building

```
make dotnet=1          # builds libraiapi2c, RaiApi2.dll and the 4 programs
```

Outputs: `FC43_x86_64/lib64/dotnet/*.dll` and launchers
`FC43_x86_64/bin/d{raisub2,raipub2,raiping2,raireplay2}` (like the Java
`j*` launchers).  `dotnet build raiapi2.sln` also works directly; pass
`-p:RaiBuildDir=<dir>` to keep `obj/` and `bin/` out of the source tree.

The rpm builds it with `rpmbuild --with dotnet` (needs `dotnet-sdk-9.0` in the
chroot).

## Programs

Ports of the Java programs, same arguments, same output:

```
draisub2   -subject TEST.REC.AAA.NaE [-snap] [-listen] [-save file] [-rate] ...
draipub2   -subject TEST.REC.AAA.NaE -data "ASK=11.0,BID=10.5" -count 3
draiping2  -subject PING.TEST -perSec 200 -msgCount 1000
draireplay2 -fileName saved.replay -perSec 10 [-realtime 1.0]
```

`-help` lists everything; `-api <name>` selects the transport as in Java.
