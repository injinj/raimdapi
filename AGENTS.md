# AGENTS.md — raimdapi

RaiApi / RaiApi2: the client API for RaiCore-style market data
(subject-based publish/subscribe with TSS/SASS typed messages and
dictionaries) in C++, C, Java (JNI), .NET and Go, with the sample tools
`raiping/raipub/raisub` (v1) and `raiping2/raipub2/raisub2/raireplay2`
(v2; `d*` = .NET, `g*` = Go, `j*` = Java builds of the same). Transports:
TIB/RV (sassrv), OMM consumer (`-api omm`, omm) and NATS (`-api nats`,
natsmd/libnatsapi). `src/msg` is the
RaiMsg message model (`RaiMsg`, `RaiField`, forms, the TSS dictionary)
inherited from RaiCore; raimd is the codec/dictionary engine underneath.
Namespace `rai` (`rai_old` for the v1 API). Depends on raikv, raimd,
sassrv, omm, natsmd, libdecnumber; Java needs a JDK (`JAVA_HOME`, or `java=0`),
.NET needs `dotnet` (`dotnet=1`).

## Workspace layout (injinj fleet convention)

- This repo is one of the `injinj` siblings. Every dependency is a sibling
  checkout (`../raikv`, `../raimd`, `../sassrv`, `../omm`, `../natsmd`,
  `../libdecnumber`); headers come from
  `-I../<dep>/include` and libraries from
  `../<dep>/FC43_x86_64/lib64/lib<dep>.a`. There is no install step between
  siblings — build the dependency, then build this repo.
- Dependencies are declared once, in `build_depends.mak`
  (`<dep>_dep := MAJOR.MINOR`, the rpm BuildRequires minimum). `./deps.sh`
  clones missing siblings from it, `./deps.sh -b` also builds them in
  dependency order, `./deps.sh -s` prints the commit of this repo and every
  sibling (the only record of what a release was built from).
- Whole-fleet build order lives in `~/injinj/GNUmakefile` (`make build`):
  openpgm → raikv → libdecnumber → raimd → linecook → lzf → rdbparser → h3 →
  HdrHistogram_c → sassrv → raids → natsmd → omm → raims → rvcache →
  raimdapi → ossl.

## Build

- `make` → `FC43_x86_64/bin`, `FC43_x86_64/lib64`. The build dir is
  `$(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)`: Fedora 43 →
  `FC43_x86_64`, Rocky 9.5 → `ROCKY9.5_x86_64`.
- `make port_extra=-g` → debug build in `FC43_x86_64-g`.
  `make port_extra=-mingw` → Windows cross build in `FC43_x86_64-mingw`
  (mingw64 toolchain; links the siblings' *master* checkouts).
- `make -j8` is safe; the makefile dependencies are complete. One trap: the
  **first `-j` run right after a compile error** can fail test executables
  with bogus undefined references (symbols from this repo's own lib)
  because they link against a library left half-updated by the failed run.
  Run plain `make` once, then `-j` works again. It is not a missing
  dependency.
- Warnings are clean. A new warning is a defect.
- If a sibling's public headers changed, rebuild the sibling first. A link
  warning like `lib<dep>.so.1.N, needed by ../<x>/.../lib<x>.so, not found`
  means sibling `<x>` was built against an older `<dep>` — rebuild `<x>`;
  at run time that warning is a hard failure (for a one-off test run,
  `ln -s ../<dep>/FC43_x86_64/lib64/lib<dep>.so /tmp/shim/lib<dep>.so.1.N`
  and `LD_LIBRARY_PATH=/tmp/shim` works, but fix the build).
- Ad hoc test program: capture the makefile's own lines once —
  `touch src/<any>.cpp; make -n | grep -- '-c src/<any>.cpp'` and
  `make -n | grep -- '-o FC43_x86_64/bin/' | head -1` — then reuse the
  compile line (minus `-c/-o`) and the link line's `-Wl,-rpath` list +
  `FC43_x86_64/lib64/libraimdapi.a` + the lib tail. If new code pulls in a
  sibling the existing binaries never needed (`DSO missing from command
  line`), append `-L../<dep>/FC43_x86_64/lib64 -l<dep>`.
- Java is on by default: the GNUmakefile derives `JAVA_HOME` from
  `javac` and errors if `jni.h` is missing — install
  `java-*-openjdk-devel`, set `JAVA_HOME`, or `make java=0`.
- `.NET`: `make dotnet=1` builds `src/raiapi/dotnet` (`RaiApi2` +
  samples; `Directory.Build.props` pins the SDK). `make golang_vet` vets
  the Go module. `make version_check` cross-checks the version strings
  in the bindings against `.copr/Makefile`.
- Windows: `make port_extra=-mingw dotnet=1 dist_win` →
  `FC43_x86_64-mingw/dist/raimdapi-<ver>-<build>-win64{.zip,-setup.exe,-debug.zip}`
  (NSIS from `win/raimdapi.nsi`; zip carries `SOURCES.txt` and
  `examples/{c++,java,dotnet}`; `-debug.zip` from `win/dist-strip.sh`).
  `make dotnet=1 dist_nuget` builds the `RaiApi2` NuGet package
  (`runtimes/win-x64/native`). Both outlive a shell timeout — run detached.

## Versioning and release

- Version is `major_num.minor_num.patch_num-build_num` in `.copr/Makefile`
  (included by the GNUmakefile). `make spec_version` prints the
  `raimdapi_dep := MAJOR.MINOR` line dependents pin in `build_depends.mak`.
- `rpm/raimdapi.spec` carries placeholders (`Version: 999.999`, release
  `99999`, `BuildRequires: <dep> _<dep>_dep`); `make srpm` substitutes them
  from `build_depends.mak`. Keep the `_ensure_git` line in
  `.copr/Makefile`: COPR's `make_srpm` chroot has no git and every
  `$(shell git …)` would silently expand empty.
- COPR: `injinj/dev` builds the `dev` branch of github.com/injinj/raimdapi
  on push (webhook); `injinj/rel` builds github.com/raitechnology/raimdapi.
  Remotes: `origin` = raitechnology (release), `ii` = injinj (dev).
  Chroots: epel-7/8/9/10, fedora-43/44/45/rawhide, x86_64.
- ABI convention: a dependent's `Requires` is `MAJOR.MINOR`, its
  `BuildRequires` the full version, and MINOR is bumped with any change to
  the C++ ABI of a public header. Bump `minor_num` here when public
  headers change, then re-pin and rebuild dependents.
- Link only against installable sibling libraries (`$(lnk_lib)`), never a
  sibling's object files.
- Signed rpms are published to https://www.raitechnology.com/repo/
  (COPR rel → download → sign → createrepo → rsync; tooling in `~/injinj/rel`).
- Windows installers and the NuGet feed are published to
  https://www.raitechnology.com/raimdapi/ and
  https://www.raitechnology.com/nuget/ (sleet; `~/injinj/nuget/up.sh`),
  plus a winget manifest (`RaiTechnology.RaiMdApi`). A new build number
  moves all three.

## Code conventions

- C++11, two-space indent, `/* … */` block comments, aligned member
  comments; snake_case for new code, but `src/msg` and the v1 API keep
  RaiCore's camelCase (`getEntry`, `packDataDictionary`) — match the file
  you are in.
- **This repo uses exceptions**: `RaiMsgErr::getErr(code)` returns an
  `ErrorRec *` and callers `throw` it; the API boundary catches and
  converts to status codes. `NO_STL` does not apply here.
- Bindings never expose `src/msg` structs: `raiapi2_c.h` / `Native.cs` /
  JNI deal in opaque handles (`rai_api_open_dict`, `rai_dict_load`,
  `rai_api_new_msg_form` …). Internal refactors of `dict.h` are invisible
  to them as long as the handle functions keep their semantics.
- Keep the generated `CMakeLists.txt` out of your edits.

## Layout

- `include/raiapi2.h`, `raiapi2_c.h` (C), `raiapi.h` (v1),
  `raisampleutil.h`; `src/raiapi/c++` (`raiapi2.cpp`, `raiapi2_tibrv.cpp`
  RV transport, `raiapi2_omm.cpp` OMM transport, `raiapi2_nats.cpp` NATS
  transport, `raiapi.cpp` v1, samples),
  `src/raiapi/c`, `src/raiapi/java` (`com/…` + JNI in `rai_msg_jni.cpp`),
  `src/raiapi/dotnet` (`RaiApi2`, `raiapi2.sln`), `src/raiapi/golang`.
- `include/msg`, `src/msg` — RaiMsg model: `msg.h`/`rai_msg.h`
  (`RaiMsg`, `RaiField`), `rai_form_msg.h` (form messages),
  `dict.h` (`RaiMsg_config`, `RaiMsg_dict`, `RaiMsg_form`),
  `mfeed_dict.h` (Marketfeed dictionary), `cfile_parser.h`/`cfile_strings.h`
  (legacy cfile parser, being replaced by raimd `CFile`), `subject.h`,
  `wildcard.h`, `string_to_msg.h`, `sass_const.h`.
- `include/base`, `src/base` — RaiCore portability layer (`Sys`, log,
  threads, time, files; `Sys::initialize()` must run before the message
  layer). `src/stream`, `src/util`.
- `test/golden_dict.cpp` — dictionary golden diff (below). `win/` —
  installer, strip script, `README-windows.md`.

## Contracts and gotchas

### Dictionary (`src/msg/dict.h`)
- `RaiMsg_config` holds `entry[]` (classes first, then each form's field
  block carved from the *end* of the array), a name hash (`hashIndex`,
  crc probe), form table, string area; shortcuts `msgType/recType/seqNo/
  recStatus` and `reservedMask` are derived from fixed fids (4001/4002/
  4003/4005).
- `fname_size` includes the NUL — the same convention as raimd's
  `MDLookup::fname_len`; copy `fname_len` bytes, do not add one.
- Form field count is not bounded by `MAX_FID` (a fid appears in many
  forms); only the *class* count is. Sanity checks must count the two
  separately.
- Form entries have `ftype RAI_TSS_NULL`; their `fsize`/`foffset`/
  shortcuts come from `RaiMsg_form::initFields(fids, n, true)`.
- TSS → RaiMsg type (`tssTypeToRaiMsgType[]`): `TSS_INTEGER` → `RAIMSG_INT`
  but `TSS_LONG` → `RAIMSG_UINT`; `DOUBLE`/`DOUBLE_INT` → `RAIMSG_REAL`;
  `OPAQUE` → `RAIMSG_OPAQUE`, `STRING`/`SDATE`/`STIME` → `RAIMSG_STRING`.
  The pack/unpack codec shares cases within each pair, so a type-pair
  swap changes `getFieldType()` but not the wire bytes.
- Packed dictionary wire format (`packDataDictionary` ≡ raimd
  `CFile::pack_sass`; sass3 variant ≡ `pack_sass3`): see raimd/AGENTS.md.
- The only dictionary consumers outside `src/msg` are `raiapi2.cpp`
  (`OpenDict`), `raiapi2_tibrv.cpp` (dict reply), `raiapi.cpp`,
  `rai_msg_jni.cpp`. `RaiMfeed_dict` is parsed/unpacked but never
  queried.
- Golden diff: `test/golden_dict.cpp` dumps every entry, form, foffset,
  shortcut and name/fid lookup from the legacy parser and from
  `fromMDDict`; build it with the captured make lines (see Build) plus
  `-I../raimd/include` and an explicit `-L../raimd/FC43_x86_64/lib64
  -lraimd`; run `golden_dict <rmds-config dir> /tmp/golden` and `diff`
  the two dumps. Known, accepted differences: form-local field names
  (`FIELD_CLASS_NAME` aliases) and the three TSS-type ambiguities listed
  in raimd/AGENTS.md.

### Messages / API
- `RaiField::Name()` on a form message returns the form-local name from
  the dictionary; this is what the `FIELD_CLASS_NAME` gap affects.
- OMM transport is consumer-only: `Publish()` returns `UNSUPPORTED_MTHD`.
  Subjects are `SERVICE.SECTOR.RIC`; `-daemon host:14002`, `-appName`,
  `-rdmPath`, `-noRdmDict`. `RAIMSG_CVT_PRECISION` env var controls real
  → string conversion precision.
- NATS transport (`raiapi2_nats.cpp`, natsmd `natsapi.h`): the tibrv
  message flow over raw NATS payloads — dictionary request on
  `_TIC.REPLY.SASS.DATA.DICTIONARY` with an inbox reply, snapshot /
  initial image request on `<snapPrefix><subject>` (`-snapPrefix _SNAP.`)
  with the sass `flags` in an RVMSG and an inbox reply, updates on the
  subject; `Publish()` sends the packed RaiMsg bytes as they are (no RV
  opaque wrapper), the receive side unwraps either raw TIBMSG/TIB_SASS/
  RVMSG or RVMSG `{_data_|_QFORM|_TIBMSG|_RAIMSG}` via
  `RaiMsg::ExtractProtocolEx()`. Args: `-daemon host:4222`, `-appName`,
  `-user`/`-pass`/`-token`, `-queueGroup` (NATS queue group for
  listeners). Interactive publish is `UNSUPPORTED_MTHD` (no
  `_RV.INFO.SYSTEM.LISTEN` on NATS). Without a dictionary server
  `raisub2` retries the dict forever, as with tibrv — use `-noDict`.
  <!-- TODO: v1 vs v2 API differences; threading model of the RV transport -->

## Tests

- Samples double as tests: `raiping2`/`raipub2`/`raisub2` against a local
  `rv_server` or `ms_server -t rv.listen`; `raireplay2` replays raimd
  captures. The Java/.NET/Go samples must produce the same output.
  <!-- TODO: scripted matrix -->
- `test/golden_dict.cpp` for any dictionary change.

## Open items / history

- 2026-10: dictionary refactor on branch `build_5` — dead code removed
  from `dict.cpp`/`mfeed_dict.cpp` (anon/dynamic forms, v1 Marketfeed
  pack/unpack, `PackedDataDictionaryFull`); `RaiMsg_config::fromMDDict()`
  (`src/msg/dict_md.cpp`) builds `dict.h` from raimd's `MDDict`;
  next: route `parseDictionary`/`unpackDataDictionary`/`packDataDictionary`
  through `CFile::parse_path`/`unpack_sass3`/`pack_sass3` and drop
  `cfile_parser.*` and the rest of `mfeed_dict.*`. Decision pending on
  form-local names and TSS type preservation in raimd.
<!-- TODO: fill from git log -->
