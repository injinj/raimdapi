# raimd makefile
lsb_dist     := $(shell if [ -f /etc/os-release ] ; then \
                  grep '^NAME=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -is ; else echo Linux ; fi)
lsb_dist_ver := $(shell if [ -f /etc/os-release ] ; then \
		  grep '^VERSION=' /etc/os-release | sed 's/.*=[\"]*//' | sed 's/[ \"].*//' ; \
                  elif [ -x /usr/bin/lsb_release ] ; then \
                  lsb_release -rs | sed 's/[.].*//' ; else uname -r | sed 's/[-].*//' ; fi)
uname_m      := $(shell uname -m)

short_dist_lc := $(patsubst CentOS,rh,$(patsubst RedHatEnterprise,rh,\
                   $(patsubst RedHat,rh,\
                     $(patsubst Fedora,fc,$(patsubst Ubuntu,ub,\
                       $(patsubst Debian,deb,$(patsubst SUSE,ss,$(lsb_dist))))))))
short_dist    := $(shell echo $(short_dist_lc) | tr a-z A-Z)
pwd           := $(shell pwd)
rpm_os        := $(short_dist_lc)$(lsb_dist_ver).$(uname_m)

# this is where the targets are compiled
build_dir ?= $(short_dist)$(lsb_dist_ver)_$(uname_m)$(port_extra)
bind        := $(build_dir)/bin
libd        := $(build_dir)/lib64
objd        := $(build_dir)/obj
dependd     := $(build_dir)/dep
java_classd := $(build_dir)/java
java_srcd   := ./src/raiapi/java

# --- language bindings ------------------------------------------------------
# java=0 / dotnet=0 disable a binding (rpm spec passes these from %bcond).
java   ?= 1
dotnet ?= 0

ifeq ($(java),1)
# Locate the JDK from whichever javac is active (follows alternatives / mock
# chroot paths like /usr/lib/jvm/java-21-openjdk-21.0.x-1.el9.x86_64).
JAVA_HOME ?= $(shell dirname $$(dirname $$(readlink -f $$(command -v javac 2>/dev/null))))
java_home := $(JAVA_HOME)
ifeq ($(wildcard $(java_home)/include/jni.h),)
  $(error jni.h not found under '$(java_home)/include' -- install java-*-openjdk-devel, set JAVA_HOME, or build with java=0)
endif
jni_os       := $(shell uname -s | tr A-Z a-z)
jni_includes := -I$(java_home)/include -I$(java_home)/include/$(jni_os)
# Emit class files loadable by this JDK's runtime.  Spec sets jdk_release per
# chroot (11 on EL7, 21 elsewhere); default = whatever javac we found.
jdk_release ?= $(shell $(java_home)/bin/javac -version 2>&1 | sed -E 's/^javac ([0-9]+).*/\1/')
javac_flags := --release $(jdk_release)
JAVAC       := $(java_home)/bin/javac $(javac_flags)
endif

empty :=
space := $(empty) $(empty)

default_cflags := -ggdb -O3
# use 'make port_extra=-g' for debug build
ifeq (-g,$(findstring -g,$(port_extra)))
  default_cflags := -ggdb
endif
ifeq (-a,$(findstring -a,$(port_extra)))
  default_cflags += -fsanitize=address
endif
ifeq (-mingw,$(findstring -mingw,$(port_extra)))
  CC    := /usr/bin/x86_64-w64-mingw32-gcc
  CXX   := /usr/bin/x86_64-w64-mingw32-g++
  mingw := true
endif
ifeq (,$(port_extra))
  build_cflags := $(shell if [ -x /bin/rpm ]; then /bin/rpm --eval '%{optflags}' ; \
                          elif [ -x /bin/dpkg-buildflags ] ; then /bin/dpkg-buildflags --get CFLAGS ; fi)
endif
# msys2 using ucrt64
ifeq (MSYS2,$(lsb_dist))
  mingw := true
endif
CC          ?= gcc
CXX         ?= g++
cc          := $(CC) -std=c11
cpp         := $(CXX)
arch_cflags := -mavx -fno-omit-frame-pointer
gcc_wflags  := -Wall -Wextra
#-Werror
# if windows cross compile
ifeq (true,$(mingw))
dll       := dll
exe       := .exe
soflag    := -shared -Wl,--subsystem,windows
fpicflags := -fPIC -DAPI_SHARED
NO_STL    := 1
else
dll       := so
exe       :=
soflag    := -shared
fpicflags := -fPIC
thread_lib  := -pthread -lrt
sock_lib    := -lcares
dynlink_lib := -lpcre2-8 -lz
endif
# make apple shared lib
ifeq (Darwin,$(lsb_dist))
dll       := dylib
endif
# rpmbuild uses RPM_OPT_FLAGS
#ifeq ($(RPM_OPT_FLAGS),)
CFLAGS ?= $(build_cflags) $(default_cflags)
#else
#CFLAGS ?= $(RPM_OPT_FLAGS)
#endif
cflags := $(gcc_wflags) $(CFLAGS) $(arch_cflags)
lflags :=

INCLUDES   ?= -Iinclude
DEFINES    ?=
includes   := $(INCLUDES)
defines    := $(DEFINES)

# if not linking libstdc++
ifdef NO_STL
cppflags    := -std=c++11 -fno-rtti -fno-exceptions
cpplink     := $(CC)
else
cppflags    := -std=c++11
cpplink     := $(CXX)
endif

.PHONY: everything
everything: all

math_lib := -lm

# test submodules exist (they don't exist for dist_rpm, dist_dpkg targets)
test_makefile = $(shell if [ -f ./$(1)/GNUmakefile ] ; then echo ./$(1) ; \
                        elif [ -f ../$(1)/GNUmakefile ] ; then echo ../$(1) ; fi)

md_home     := $(call test_makefile,raimd)
dec_home    := $(call test_makefile,libdecnumber)
kv_home     := $(call test_makefile,raikv)
sassrv_home := $(call test_makefile,sassrv)
omm_home    := $(call test_makefile,omm)

ifeq (,$(dec_home))
dec_home    := $(call test_makefile,$(md_home)/libdecnumber)
endif

lnk_lib     := -Wl,--push-state -Wl,-Bstatic
dlnk_lib    :=
lnk_dep     :=
dlnk_dep    :=

# omm first: it depends on sassrv/raimd/raikv below
ifneq (,$(omm_home))
omm_lib     := $(omm_home)/$(libd)/libomm.a
omm_dll     := $(omm_home)/$(libd)/libomm.$(dll)
lnk_lib     += $(omm_lib)
lnk_dep     += $(omm_lib)
dlnk_lib    += -L$(omm_home)/$(libd) -lomm
dlnk_dep    += $(omm_dll)
rpath5       = ,-rpath,$(pwd)/$(omm_home)/$(libd)
includes    += -I$(omm_home)/include
else
lnk_lib     += $(push_static) -lomm $(pop_static)
dlnk_lib    += -lomm
endif

ifneq (,$(sassrv_home))
sassrv_lib  := $(sassrv_home)/$(libd)/libsassrv.a
sassrv_dll  := $(sassrv_home)/$(libd)/libsassrv.$(dll)
rv7lib_lib  := $(sassrv_home)/$(libd)/librv7lib.a
rv7lib_dll  := $(sassrv_home)/$(libd)/librv7lib.$(dll)
lnk_lib     += $(rv7lib_lib) $(sassrv_lib)
lnk_dep     += $(rv7lib_lib) $(sassrv_lib)
dlnk_lib    += -L$(sassrv_home)/$(libd) -lrv7lib -lsassrv
dlnk_dep    += $(rv7lib_dll) $(sassrv_dll)
rpath4       = ,-rpath,$(pwd)/$(sassrv_home)/$(libd)
includes    += -I$(sassrv_home)/include
else
lnk_lib     += $(push_static) -lrv7lib -lsassrv $(pop_static)
dlnk_lib    += -lrv7lib -lsassrv
endif

ifneq (,$(md_home))
md_lib      := $(md_home)/$(libd)/libraimd.a
md_dll      := $(md_home)/$(libd)/libraimd.$(dll)
lnk_lib     += $(md_lib)
lnk_dep     += $(md_lib)
dlnk_lib    += -L$(md_home)/$(libd) -lraimd
dlnk_dep    += $(md_dll)
rpath1       = ,-rpath,$(pwd)/$(md_home)/$(libd)
includes    += -I$(md_home)/include
else
lnk_lib     += $(push_static) -lraimd $(pop_static)
dlnk_lib    += -lraimd
endif

ifneq (,$(dec_home))
dec_lib     := $(dec_home)/$(libd)/libdecnumber.a
dec_dll     := $(dec_home)/$(libd)/libdecnumber.$(dll)
lnk_lib     += $(dec_lib)
lnk_dep     += $(dec_lib)
dlnk_lib    += -L$(dec_home)/$(libd) -ldecnumber
dlnk_dep    += $(dec_dll)
rpath2       = ,-rpath,$(pwd)/$(dec_home)/$(libd)
dec_includes = -I$(dec_home)/include
else
lnk_lib     += $(push_static) -ldecnumber $(pop_static)
dlnk_lib    += -ldecnumber
endif

ifneq (,$(kv_home))
kv_lib      := $(kv_home)/$(libd)/libraikv.a
kv_dll      := $(kv_home)/$(libd)/libraikv.$(dll)
lnk_lib     += $(kv_lib)
lnk_dep     += $(kv_lib)
dlnk_lib    += -L$(kv_home)/$(libd) -lraikv
dlnk_dep    += $(kv_dll)
rpath3       = ,-rpath,$(pwd)/$(kv_home)/$(libd)
includes    += -I$(kv_home)/include
else
lnk_lib     += $(push_static) -lraikv $(pop_static)
dlnk_lib    += -lraikv
endif

rpath   := -Wl,-rpath,$(pwd)/$(libd)$(rpath1)$(rpath2)$(rpath3)$(rpath4)$(rpath5)
lnk_lib += -Wl,--pop-state

.PHONY: everything
everything: $(kv_lib) $(dec_lib) $(md_lib) $(sassrv_lib) $(omm_lib) all

clean_subs :=
# build submodules if have them
ifneq (,$(kv_home))
$(kv_lib) $(kv_dll):
	$(MAKE) -C $(kv_home)
.PHONY: clean_kv
clean_kv:
	$(MAKE) -C $(kv_home) clean
clean_subs += clean_kv
endif
ifneq (,$(dec_home))
$(dec_lib) $(dec_dll):
	$(MAKE) -C $(dec_home)
.PHONY: clean_dec
clean_dec:
	$(MAKE) -C $(dec_home) clean
clean_subs += clean_dec
endif
ifneq (,$(md_home))
$(md_lib) $(md_dll):
	$(MAKE) -C $(md_home)
.PHONY: clean_md
clean_md:
	$(MAKE) -C $(md_home) clean
clean_subs += clean_md
endif
ifneq (,$(sassrv_home))
$(sassrv_lib) $(sassrv_dll):
	$(MAKE) -C $(sassrv_home)
.PHONY: clean_sassrv
clean_sassrv:
	$(MAKE) -C $(sassrv_home) clean
clean_subs += clean_sassrv
endif
ifneq (,$(omm_home))
$(omm_lib) $(omm_dll):
	$(MAKE) -C $(omm_home)
.PHONY: clean_omm
clean_omm:
	$(MAKE) -C $(omm_home) clean
clean_subs += clean_omm
endif

# copr/fedora build (with version env vars)
# copr uses this to generate a source rpm with the srpm target
-include .copr/Makefile
# debian build (debuild)
# target for building installable deb: dist_dpkg
-include deb/Makefile

all_exes    :=
all_libs    :=
all_dlls    :=
all_depends :=

raiapi_files    := raiapi
base_files      := sys time log file thread mem dir
stream_files    := io_stream file_stream stdio_stream byte_array_stream cycle_stream
util_files      := snprintf strptime hash_util uri str_util args
libraiapi_files := $(raiapi_files) $(base_files) $(stream_files) $(util_files)
libraiapi_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraiapi_files)))
libraiapi_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraiapi_files)))
libraiapi_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraiapi_files))) \
                   $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraiapi_files)))
libraiapi_dlnk  := $(dlnk_lib)
libraiapi_spec  := $(ver_build)_$(git_hash)
libraiapi_ver   := $(major_num).$(minor_num)

$(libd)/libraiapi.a: $(libraiapi_objs)
$(libd)/libraiapi.$(dll): $(libraiapi_dbjs) $(lnk_dep) $(dlnk_dep)

all_libs    += $(libd)/libraiapi.a
all_dlls    += $(libd)/libraiapi.$(dll)
all_depends += $(libraiapi_deps)

raiapi2_files   := raiapi2 raiapi2_tibrv
base_files      := sys time log file thread mem dir
stream_files    := io_stream file_stream stdio_stream byte_array_stream cycle_stream
util_files      := snprintf strptime hash_util uri str_util args
libraiapi2_files := $(raiapi2_files) $(base_files) $(stream_files) $(util_files)
libraiapi2_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraiapi2_files)))
libraiapi2_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraiapi2_files)))
libraiapi2_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraiapi2_files))) \
                   $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraiapi2_files)))
libraiapi2_dlnk  := $(dlnk_lib)
libraiapi2_spec  := $(ver_build)_$(git_hash)
libraiapi2_ver   := $(major_num).$(minor_num)

$(libd)/libraiapi2.a: $(libraiapi2_objs)
$(libd)/libraiapi2.$(dll): $(libraiapi2_dbjs) $(lnk_dep) $(dlnk_dep)

all_libs    += $(libd)/libraiapi2.a
all_dlls    += $(libd)/libraiapi2.$(dll)
all_depends += $(libraiapi2_deps)

libraimsg_files := cfile_parser msg field dict subject sass_const wildcard mfeed_dict rai_form_msg
libraimsg_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(libraimsg_files)))
libraimsg_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libraimsg_files)))
libraimsg_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(libraimsg_files))) \
                   $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libraimsg_files)))
libraimsg_dlnk  :=
libraimsg_spec  := $(ver_build)_$(git_hash)
libraimsg_ver   := $(major_num).$(minor_num)

$(libd)/libraimsg.a: $(libraimsg_objs)
$(libd)/libraimsg.$(dll): $(libraimsg_dbjs)

all_libs    += $(libd)/libraimsg.a
all_dlls    += $(libd)/libraimsg.$(dll)
all_depends += $(libraimsg_deps)

raiapi_lib  := $(libd)/libraiapi.a $(libd)/libraimsg.a
raiapi_lnk  := $(libd)/libraiapi.a $(libd)/libraimsg.a $(lnk_lib)
raiapi_dlib := $(libd)/libraiapi.$(dll) $(libd)/libraimsg.$(dll)
raiapi_dlnk := -lraiapi -lraimsg

raisub_files := raisub raisampleutil
raisub_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raisub_files)))
raisub_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raisub_files)))
raisub_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raisub_files)))
raisub_libs  := $(raiapi_dlib)
raisub_lnk   := $(raiapi_dlnk)

$(bind)/raisub$(exe): $(raisub_objs) $(raisub_libs)
all_exes += $(bind)/raisub$(exe)
all_depends +=  $(raisub_deps)

raipub_files := raipub raisampleutil
raipub_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raipub_files)))
raipub_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raipub_files)))
raipub_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raipub_files)))
raipub_libs  := $(raiapi_dlib)
raipub_lnk   := $(raiapi_dlnk)

$(bind)/raipub$(exe): $(raipub_objs) $(raipub_libs)
all_exes += $(bind)/raipub$(exe)
all_depends +=  $(raipub_deps)

raiping_files := raiping raisampleutil
raiping_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raiping_files)))
raiping_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raiping_files)))
raiping_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raiping_files)))
raiping_libs  := $(raiapi_dlib)
raiping_lnk   := $(raiapi_dlnk)

$(bind)/raiping$(exe): $(raiping_objs) $(raiping_libs)
all_exes += $(bind)/raiping$(exe)
all_depends +=  $(raiping_deps)

raiapi2_lib  := $(libd)/libraiapi2.a $(libd)/libraimsg.a
raiapi2_lnk  := $(libd)/libraiapi2.a $(libd)/libraimsg.a $(lnk_lib)
raiapi2_dlib := $(libd)/libraiapi2.$(dll) $(libd)/libraimsg.$(dll)
raiapi2_dlnk := -lraiapi2 -lraimsg

raisub2_files := raisub2
raisub2_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raisub2_files)))
raisub2_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raisub2_files)))
raisub2_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raisub2_files)))
raisub2_libs  := $(raiapi2_dlib)
raisub2_lnk   := $(raiapi2_dlnk)

$(bind)/raisub2$(exe): $(raisub2_objs) $(raisub2_libs)
all_exes += $(bind)/raisub2$(exe)
all_depends +=  $(raisub2_deps)

raipub2_files := raipub2
raipub2_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raipub2_files)))
raipub2_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raipub2_files)))
raipub2_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raipub2_files)))
raipub2_libs  := $(raiapi2_dlib)
raipub2_lnk   := $(raiapi2_dlnk)

$(bind)/raipub2$(exe): $(raipub2_objs) $(raipub2_libs)
all_exes += $(bind)/raipub2$(exe)
all_depends +=  $(raipub2_deps)

raireplay2_files := raireplay2
raireplay2_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raireplay2_files)))
raireplay2_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raireplay2_files)))
raireplay2_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raireplay2_files)))
raireplay2_libs  := $(raiapi2_dlib)
raireplay2_lnk   := $(raiapi2_dlnk)

$(bind)/raireplay2$(exe): $(raireplay2_objs) $(raireplay2_libs)
all_exes += $(bind)/raireplay2$(exe)
all_depends +=  $(raireplay2_deps)

raiping2_files := raiping2
raiping2_cfile := $(addprefix src/raiapi/c++/, $(addsuffix .cpp, $(raiping2_files)))
raiping2_objs  := $(addprefix $(objd)/, $(addsuffix .o, $(raiping2_files)))
raiping2_deps  := $(addprefix $(dependd)/, $(addsuffix .d, $(raiping2_files)))
raiping2_libs  := $(raiapi2_dlib)
raiping2_lnk   := $(raiapi2_dlnk)

$(bind)/raiping2$(exe): $(raiping2_objs) $(raiping2_libs)
all_exes += $(bind)/raiping2$(exe)
all_depends +=  $(raiping2_deps)

rai_api_jni_includes = $(jni_includes) -Isrc
libjraiapi2_files := rai_api_jni
libjraiapi2_objs  := $(addprefix src/raiapi/c++/java/com/rai/raiapi2/, $(addsuffix .cpp, $(libjraiapi2_files)))
libjraiapi2_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libjraiapi2_files)))
libjraiapi2_deps  := $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libjraiapi2_files)))
libjraiapi2_dlnk  := -L$(libd) $(raiapi2_dlnk) $(dlnk_lib)
libjraiapi2_spec  := $(ver_build)_$(git_hash)
libjraiapi2_ver   := $(major_num).$(minor_num)

$(libd)/libjraiapi2.$(dll): $(libjraiapi2_dbjs) $(raiapi2_dlib) $(lnk_dep) $(dlnk_dep)

rai_msg_jni_includes = $(jni_includes) -Isrc
libjraimsg_files := rai_msg_jni
libjraimsg_objs  := $(addprefix src/raiapi/c++/java/com/rai/raiapi2/, $(addsuffix .cpp, $(libjraimsg_files)))
libjraimsg_dbjs  := $(addprefix $(objd)/, $(addsuffix .fpic.o, $(libjraimsg_files)))
libjraimsg_deps  := $(addprefix $(dependd)/, $(addsuffix .fpic.d, $(libjraimsg_files)))
libjraimsg_dlnk  := -L$(libd) -lraimsg $(dlnk_lib)
libjraimsg_spec  := $(ver_build)_$(git_hash)
libjraimsg_ver   := $(major_num).$(minor_num)

$(libd)/libjraimsg.$(dll): $(libjraimsg_dbjs) $(libd)/libraimsg.$(dll) $(lnk_dep) $(dlnk_dep)

ifeq ($(java),1)
all_dlls    += $(libd)/libjraiapi2.$(dll) $(libd)/libjraimsg.$(dll)
all_depends += $(libjraiapi2_deps) $(libjraimsg_deps)
endif

raiexception_root     = com/rai/raiexception
raiexception_classes  = $(java_classd)/$(raiexception_root)/RaiException.class
raiexception_manifest = $(java_classd)/raiexception_manifest.txt
$(libd)/raiexception.jar: $(raiexception_classes) $(raiexception_manifest)

ifeq ($(java),1)
all_libs += $(libd)/raiexception.jar
endif

raimsg_root     = com/rai/raimsg
raimsg_classes  = $(java_classd)/$(raimsg_root)/RaiMsg.class $(java_classd)/$(raimsg_root)/RaiField.class \
                  $(java_classd)/$(raimsg_root)/Partial.class $(java_classd)/$(raimsg_root)/SassConst.class \
		  $(java_classd)/$(raimsg_root)/RaiMsgException.class
raimsg_manifest = $(java_classd)/raimsg_manifest.txt
$(libd)/raimsg.jar: $(raimsg_classes) $(raimsg_manifest)

ifeq ($(java),1)
all_libs += $(libd)/raimsg.jar
endif

raiapi_root    = com/rai/raiapi
raiapi_classes = $(java_classd)/$(raiapi_root)/RaiApi.class $(java_classd)/$(raiapi_root)/RaiCallback.class \
                 $(java_classd)/$(raiapi_root)/RaiDict.class $(java_classd)/$(raiapi_root)/RaiEvent.class \
                 $(java_classd)/$(raiapi_root)/RaiException.class $(java_classd)/$(raiapi_root)/RaiPublish.class \
                 $(java_classd)/$(raiapi_root)/RaiSession.class $(java_classd)/$(raiapi_root)/RaiSubscribe.class \
                 $(java_classd)/$(raiapi_root)/RaiSession.class $(java_classd)/$(raiapi_root)/RaiTimerCallback.class \
                 $(java_classd)/$(raiapi_root)/RaiTimer.class $(java_classd)/$(raiapi_root)/SubHandle.class \
                 $(java_classd)/$(raiapi_root)/TimerHandle.class
raiapi_manifest = $(java_classd)/raiapi_manifest.txt
$(libd)/raiapi.jar: $(raiapi_classes) $(raiapi_manifest)

ifeq ($(java),1)
all_libs += $(libd)/raiapi.jar
endif

raiapi2_root    = com/rai/raiapi2
raiapi2_classes = $(java_classd)/$(raiapi2_root)/Args.class $(java_classd)/$(raiapi2_root)/RaiMsgEvent.class \
                  $(java_classd)/$(raiapi2_root)/BoolArg.class $(java_classd)/$(raiapi2_root)/RaiPublish.class \
                  $(java_classd)/$(raiapi2_root)/DoubleArg.class $(java_classd)/$(raiapi2_root)/RaiQueue.class \
                  $(java_classd)/$(raiapi2_root)/IntArg.class $(java_classd)/$(raiapi2_root)/RaiApiException.class \
                  $(java_classd)/$(raiapi2_root)/RaiApi.class $(java_classd)/$(raiapi2_root)/RaiSession.class \
                  $(java_classd)/$(raiapi2_root)/RaiSubscribeCallback.class $(java_classd)/$(raiapi2_root)/RaiSubscribeEvent.class \
                  $(java_classd)/$(raiapi2_root)/RaiConnectionEvent.class $(java_classd)/$(raiapi2_root)/RaiSubscribe.class \
                  $(java_classd)/$(raiapi2_root)/RaiDataLossCallback.class $(java_classd)/$(raiapi2_root)/RaiTimerCallback.class \
                  $(java_classd)/$(raiapi2_root)/RaiDataLossEvent.class $(java_classd)/$(raiapi2_root)/RaiTimer.class \
                  $(java_classd)/$(raiapi2_root)/RaiDict.class $(java_classd)/$(raiapi2_root)/StringArg.class \
                  $(java_classd)/$(raiapi2_root)/RaiEntitlement.class $(java_classd)/$(raiapi2_root)/Time.class \
                  $(java_classd)/$(raiapi2_root)/RaiInteractivePublish.class $(java_classd)/$(raiapi2_root)/TimeRotate.class \
                  $(java_classd)/$(raiapi2_root)/RaiMsgCallback.class
raiapi2_manifest = $(java_classd)/raiapi2_manifest.txt
$(libd)/raiapi2.jar: $(raiapi2_classes) $(raiapi2_manifest)

ifeq ($(java),1)
all_libs += $(libd)/raiapi2.jar
endif

# --- java programs ----------------------------------------------------------
# Each program <p> compiles into its own staging dir $(java_classd)/<p>/ and is
# packaged as $(libd)/<p>.jar, so helper/nested classes (Args, $SubThread...)
# stay out of lib64.  Launcher $(bind)/j<p> puts lib64 + every lib64 jar on the
# classpath and names the main class.  Per program, declare only its jar deps:
#     <p>_jars := ...
# then add $(bind)/j<p> to java_progs.
raisub_jars  := $(libd)/raiapi.jar  $(libd)/raimsg.jar $(libd)/raiexception.jar
raisub2_jars := $(libd)/raiapi2.jar $(libd)/raimsg.jar $(libd)/raiexception.jar

java_progs := $(bind)/jraisub $(bind)/jraisub2
java_prog_names := $(patsubst $(bind)/j%,%,$(java_progs))
java_prog_jars  := $(addprefix $(libd)/, $(addsuffix .jar, $(java_prog_names)))

# stamp file = "all classes for <p> compiled into $(java_classd)/<p>/"
.SECONDEXPANSION:
$(java_classd)/%.stamp: $(java_srcd)/%.java $$($$*_jars)
	rm -rf $(java_classd)/$* && mkdir -p $(java_classd)/$*
	$(JAVAC) -classpath $(subst $(space),:,$($*_jars)) -d $(java_classd)/$* $<
	@touch $@

# program jar: everything the compile produced, whatever the class names.
# Static pattern rule so it applies ONLY to program jars, never the API jars
# built by the generic $(libd)/%.jar rule below.
$(java_prog_jars): $(libd)/%.jar: $(java_classd)/%.stamp
	jar cf $@ -C $(java_classd)/$* .

$(bind)/j%: $(libd)/%.jar GNUmakefile
	@{ echo '#!/bin/sh'; \
	   echo '# generated by GNUmakefile - do not edit'; \
	   echo 'here=$$(cd "$$(dirname "$$0")/.." && pwd)'; \
	   echo 'lib="$$here/lib64"'; \
	   echo 'cp="$$lib"; for j in "$$lib"/*.jar; do cp="$$cp:$$j"; done'; \
	   echo '# JNI libs (libjraimsg.so, libjraiapi2.so) live beside the jars'; \
	   echo 'exec java -Djava.library.path="$$lib" -classpath "$$cp" $* "$$@"'; } > $@
	chmod +x $@

ifeq ($(java),1)
all_exes += $(java_progs)
endif

all_dirs := $(bind) $(libd) $(objd) $(dependd) $(java_classd)

all: $(all_libs) $(all_dlls) $(all_exes)

$(libd)/%.jar:
	jar cvfm $@ $($(*)_manifest) $(addprefix -C $(java_classd) $($(*)_root)/, $(notdir $($(*)_classes)))

$(java_classd)/%.class: $(java_srcd)/%.java
	$(JAVAC) -sourcepath $(java_srcd) -classpath $(java_classd) $< -d $(java_classd)

$(java_classd)/%_manifest.txt:
	echo "Implementation-Title:" $(notdir $*) > $@
	echo "Implementation-Version:" 1.0 >> $@
	echo "Implementation-Vendor: http://www.raitechnology.com/" >> $@

# create directories
$(dependd):
	@mkdir -p $(all_dirs)

# remove target bins, objs, depends
.PHONY: clean
clean:
	rm -r -f $(bind) $(libd) $(objd) $(dependd)
	if [ "$(build_dir)" != "." ] ; then rmdir $(build_dir) ; fi

.PHONY: clean_dist
clean_dist:
	rm -rf dpkgbuild rpmbuild

.PHONY: clean_all
clean_all: clean clean_dist

$(dependd)/depend.make: $(dependd) $(all_depends)
	@echo "# depend file" > $(dependd)/depend.make
	@cat $(all_depends) >> $(dependd)/depend.make

ifeq (SunOS,$(lsb_dist))
remove_rpath = rpath -r
else
remove_rpath = chrpath -d
endif
# target used by rpmbuild, dpkgbuild
.PHONY: dist_bins
dist_bins: $(all_libs) $(all_dlls) $(bind)/raisub2$(exe) $(if $(filter 1,$(java)),$(java_progs))
	$(remove_rpath) $(bind)/raisub$(exe)
	$(remove_rpath) $(libd)/libraiapi.$(dll)

# target for building installable rpm
.PHONY: dist_rpm
dist_rpm: srpm
	( cd rpmbuild && rpmbuild --define "-topdir `pwd`" -ba SPECS/raimd.spec )

# force a remake of depend using 'make -B depend'
.PHONY: depend
depend: $(dependd)/depend.make

# dependencies made by 'make depend'
-include $(dependd)/depend.make

ifeq ($(DESTDIR),)
# 'sudo make install' puts things in /usr/local/lib, /usr/local/include
install_prefix ?= /usr/local
else
# debuild uses DESTDIR to put things into debian/libdecnumber/usr
install_prefix = $(DESTDIR)/usr
endif
# this should be 64 for rpm based, /64 for SunOS
install_lib_suffix ?=

install: dist_bins
	install -d $(install_prefix)/lib$(install_lib_suffix)
	install -d $(install_prefix)/bin $(install_prefix)/include/raiapi
	for f in $(libd)/libraiapi.* ; do \
	if [ -h $$f ] ; then \
	cp -a $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	else \
	install $$f $(install_prefix)/lib$(install_lib_suffix) ; \
	fi ; \
	done
	install -m 644 include/raiapi/*.h $(install_prefix)/include/raiapi

$(objd)/%.o: src/raiapi/c++/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/base/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/util/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/stream/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/msg/%.cpp
	$(cpp) $(cflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.o: src/util/%.c
	$(cc) $(cflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/raiapi/java/com/rai/raiapi2/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/raiapi/java/com/rai/raimsg/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/raiapi/c++/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/base/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/util/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/stream/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/msg/%.cpp
	$(cpp) $(cflags) $(fpicflags) $(cppflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(objd)/%.fpic.o: src/util/%.c
	$(cc) $(cflags) $(fpicflags) $(includes) $(defines) $($(notdir $*)_includes) $($(notdir $*)_defines) -c $< -o $@

$(libd)/%.a:
	ar rc $@ $($(*)_objs)

ifeq (Darwin,$(lsb_dist))
$(libd)/%.dylib:
	$(cpplink) -dynamiclib $(cflags) $(lflags) -o $@.$($(*)_dylib).dylib -current_version $($(*)_dylib) -compatibility_version $($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_dylib).dylib $(@F).$($(*)_ver).dylib && ln -f -s $(@F).$($(*)_ver).dylib $(@F)
else
$(libd)/%.$(dll):
	$(cpplink) $(soflag) $(rpath) $(cflags) $(lflags) -o $@.$($(*)_spec) -Wl,-soname=$(@F).$($(*)_ver) $($(*)_dbjs) $($(*)_dlnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib) && \
	cd $(libd) && ln -f -s $(@F).$($(*)_spec) $(@F).$($(*)_ver) && ln -f -s $(@F).$($(*)_ver) $(@F)
endif

$(bind)/%$(exe):
	$(cpplink) $(cflags) $(lflags) $(rpath) -o $@ $($(*)_objs) -L$(libd) $($(*)_lnk) $(cpp_lnk) $(sock_lib) $(math_lib) $(thread_lib) $(malloc_lib) $(dynlink_lib)

$(dependd)/%.d: src/raiapi/c++/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/base/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/util/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/stream/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/msg/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.d: src/util/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).o -MF $@

$(dependd)/%.fpic.d: src/raiapi/c++/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/base/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/util/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/stream/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/msg/%.cpp
	$(cpp) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

$(dependd)/%.fpic.d: src/util/%.c
	$(cc) $(arch_cflags) $(defines) $(includes) $($(notdir $*)_includes) $($(notdir $*)_defines) -MM $< -MT $(objd)/$(*).fpic.o -MF $@

