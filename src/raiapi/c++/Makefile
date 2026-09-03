#
# GNU Makefile (gmake) to build rai C++ sample code
#
# make -- makes everything
# make palo_alto_all -- makes Palo Alto RaiAPI2 programs
# make santa_clara_all -- makes Santa Clara RaiAPI programs (deprecated)
# make service_all -- makes Palo Alto RaiAPI2 service dll modules

# Change to point to the Rai installation directory
RAI_DIR=/usr/rai
# If above isn't installed, then this will get include and lib from c++ dir
REL_RAI_DIR=../..

# Change to point to the TIBCO Rendezvous >= 7 installation directory
TIBRV_DIR=/usr/tibco/tibrv

# Uncomment the following to build 32 bit binaries
# build_64=0

# Default compiler and output options
O=.o
LIB=.a
EXE=
DLL=.so
FPIC=.fpic.o
RM=rm -f
OUT=-o 
DASH_C=-c

# Determine architecture specific settings
uname_m = $(shell uname -m)
uname_s = $(shell uname -s)
ifneq (,$(findstring SunOS,$(uname_s)))
  arch=solaris
else
  arch=linux
endif
# set build_64=0 if building 32 bit
ifeq (,$(build_64))
  ifneq (i686,$(uname_m))
    build_64=1
  endif
endif
ifeq (1,$(build_64))
  port_bits=64
  RV64=64
  MARCH=-m64
  L32=
else
  RV64=
  MARCH=-m32
  L32=32
endif

# optimizations
OPT_FLAG=-g
ifeq (solaris,$(arch))
#  SOFLAG=-G
#  FPIC_FLAGS=-DRAI_DLL_EMBEDDED -KPIC
#  CXX=CC
#  CXXFLAGS=$(OPT_FLAG) -features=zla -erroff=wvarhidemem
#  LINK=CC
#  CXXLIB=-lCstd -lCrun
#  THR_FLAG=-mt
  SOFLAG=-shared
  FPIC_FLAGS=-DRAI_DLL_EMBEDDED -fPIC
  CXX=g++
  CXXFLAGS=$(OPT_FLAG)
  LINK=gcc
  CXXLIB=-lstdc++
  THR_FLAG=-pthread
  DEFINES=-Wno-deprecated
else
  SOFLAG=-shared
  FPIC_FLAGS=-DRAI_DLL_EMBEDDED -fPIC
  CXX=g++
  CXXFLAGS=$(OPT_FLAG)
  # IF using libstdc++ instead of libsupc++ change LINK=g++ and remove -lsupc++
  # from OS_LIBS
  LINK=gcc
  CXXLIB=-lsupc++
  THR_FLAG=-pthread
  DEFINES=-Wno-deprecated
endif

ifeq ($(arch),solaris)
  DEFINES += -D_XOPEN_SOURCE_EXTENDED=1 -D_POSIX_PTHREAD_SEMANTICS -DNO_WHATLIST
  OS_LIBS = $(CXXLIB) -lsocket -lnsl -lpthread -lrt -lm
else
  OS_LIBS = $(CXXLIB) -lpthread -lrt -lm
endif

# These are the static RAI libs that the version 1 apis link
SC_RAI_LIBS= -L$(REL_RAI_DIR)/lib$(L32) -L$(RAI_DIR)/lib$(L32) -lraiapi -lraimsg -lbase
# These are the dynamic RAI libs that the version 2 apis link
PA_RAI_LIBS= -L$(REL_RAI_DIR)/dll$(L32) -L$(RAI_DIR)/dll$(L32) -lraiapi2 -lraimsg -lraibase
# These are the static RAI libs that the version 2 apis link
PA_RAI_STATIC_LIBS= -L$(REL_RAI_DIR)/lib$(L32) -L$(RAI_DIR)/lib$(L32) -lraiapi2 -lmessage -lraimsg -lmdmsg -ldoc -lci -lnet -lbase -lrsslData -lbz2 -llz4 -lz
RAI_INC= -I$(REL_RAI_DIR)/include -I$(RAI_DIR)/include

# These are the dynamic library version info for the 'make service_all' target,
# raicache will load RAI_MAJOR.RAI_MINOR services, so RAI_PATCH and RAI_BUILD
# can change locally, the last 2 numbers of the RAI_SERVICE_BUILD
RAI_SERVICE_VERSION=2.11
RAI_SERVICE_BUILD=2.11.0.140
RAI_DLL_OUT=$(OUT)$(@F).$(RAI_SERVICE_BUILD)
SYM_LINK_VERS=&& ln -s -f $(@F).$(RAI_SERVICE_BUILD) $(@F).$(RAI_SERVICE_VERSION) && ln -s -f $(@F).$(RAI_SERVICE_VERSION) $(@F)
ifeq ($(arch),linux)
  LINK_VERS=-Wl,-soname=$(@F).$(RAI_SERVICE_VERSION)
else
  LINK_VERS=-h $(@F).$(RAI_SERVICE_VERSION)
endif

# These are the Tibco libs the santa clara apis need
RV_LIBS= -L$(TIBRV_DIR)/lib -ltibrv$(RV64)

# the santa clara version 1 api uses tibrv libs
santa_clara_api= raipub$(EXE) raisub$(EXE) raiping$(EXE)

# the palo alto version 2 api dynamically loads protocols
palo_alto_api= raipub2$(EXE) raisub2$(EXE) raiping2$(EXE) raireplay2$(EXE) \
               feedstats$(EXE) msg2csv$(EXE)

# the palo alto statically linked api
palo_alto_static_api=raipub2_static$(EXE) raisub2_static$(EXE) \
                     raiping2_static$(EXE) raireplay2_static$(EXE) \
		     feedstats_static$(EXE) msg2csv_static$(EXE)

# the palo alto dll raicache services
palo_alto_service_api=rai_service_raipub2$(DLL) rai_service_raisub2$(DLL) \
                      rai_service_raiping2$(DLL) rai_service_raireplay2$(DLL) \
		      rai_service_feedstats$(DLL) rai_service_msg2csv$(DLL)

all: palo_alto_all santa_clara_all service_all

palo_alto_all: $(palo_alto_api) $(palo_alto_static_api) 

santa_clara_all: $(santa_clara_api)

# these require dll/raicache
service_all: $(palo_alto_service_api)

# santa clara api sources and bins
raipub$(O): raipub.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raisub$(O): raisub.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raiping$(O): raiping.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raisampleutil$(O): raisampleutil.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raipub$(EXE): raipub$(O) raisampleutil$(O) 
	$(LINK) $(MARCH) $(LDFLAGS) raipub$(O) raisampleutil$(O) $(SC_RAI_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

raisub$(EXE): raisub$(O) raisampleutil$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raisub$(O) raisampleutil$(O) $(SC_RAI_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

raiping$(EXE): raiping$(O) raisampleutil$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raiping$(O) raisampleutil$(O) $(SC_RAI_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

# palo alto api version2 sources and bins
raipub2$(O): raipub2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raisub2$(O): raisub2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raireplay2$(O): raireplay2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

raiping2$(O): raiping2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

feedstats$(O): feedstats.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

msg2csv$(O): msg2csv.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C)

# palo alto api version2 fpic dll embedded version
raipub2$(FPIC): raipub2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

raisub2$(FPIC): raisub2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

raireplay2$(FPIC): raireplay2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

raiping2$(FPIC): raiping2.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

feedstats$(FPIC): feedstats.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

msg2csv$(FPIC): msg2csv.cpp
	$(CXX) $(MARCH) $(THR_FLAG) $(FPIC_FLAGS) $(RAI_INC) $(DEFINES) $< $(CXXFLAGS) $(DASH_C) $(OUT)$@

# palo alto dynamic linked version
raipub2$(EXE): raipub2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raipub2$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

raisub2$(EXE): raisub2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raisub2$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

raiping2$(EXE): raiping2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raiping2$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

raireplay2$(EXE): raireplay2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raireplay2$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

feedstats$(EXE): feedstats$(O)
	$(LINK) $(MARCH) $(LDFLAGS) feedstats$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

msg2csv$(EXE): msg2csv$(O)
	$(LINK) $(MARCH) $(LDFLAGS) msg2csv$(O) $(PA_RAI_LIBS) $(OS_LIBS) $(OUT)$@

# palo alto static linked version
raipub2_static$(EXE): raipub2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raipub2$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

raisub2_static$(EXE): raisub2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raisub2$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

raiping2_static$(EXE): raiping2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raiping2$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

raireplay2_static$(EXE): raireplay2$(O)
	$(LINK) $(MARCH) $(LDFLAGS) raireplay2$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

feedstats_static$(EXE): feedstats$(O)
	$(LINK) $(MARCH) $(LDFLAGS) feedstats$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

msg2csv_static$(EXE): msg2csv$(O)
	$(LINK) $(MARCH) $(LDFLAGS) msg2csv$(O) $(PA_RAI_STATIC_LIBS) $(RV_LIBS) $(OS_LIBS) $(OUT)$@

# palo alto dll raicache embedded version
rai_service_raipub2$(DLL): raipub2$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) raipub2$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

rai_service_raisub2$(DLL): raisub2$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) raisub2$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

rai_service_raiping2$(DLL): raiping2$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) raiping2$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

rai_service_raireplay2$(DLL): raireplay2$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) raireplay2$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

rai_service_feedstats$(DLL): feedstats$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) feedstats$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

rai_service_msg2csv$(DLL): msg2csv$(FPIC)
	$(LINK) $(MARCH) $(SOFLAG) msg2csv$(FPIC) $(PA_RAI_LIBS) $(OS_LIBS) $(RAI_DLL_OUT) $(LINK_VERS) $(SYM_LINK_VERS)

clean: 	
	$(RM) *$(O)
	$(RM) core
	$(RM) $(palo_alto_api) $(palo_alto_static_api) $(santa_clara_api) *$(DLL)*

