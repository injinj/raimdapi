#
# libjraiapi2
#
jraiapi2_root = com/rai/raiapi2

ifneq (win64,$(arch))
rai_api_jni_includes = -I$(java_home)/include -I$(java_home)/include/$(arch)
else
rai_api_jni_includes = -I$(java_home)/include -I$(java_home)/include/win32
endif

ifeq (32,$(port_bits))
java_build32_bits=true
endif
ifeq (64_32,$(build_bits))
java_build32_bits=true
endif

ifeq (true,$(java_build32_bits))
libjraiapi2_objs  = $(java_classd)/$(jraiapi2_root)/rai_api_jni$(fpic32)
libjraiapi2_deps  = $(src_dependd)/raiapi/java/$(jraiapi2_root)/rai_api_jni.fpic.32.d

ifdef unix
libjraiapi2_libs  = 
libjraiapi2_lnk   = $(math_lib) $(thread_lib)
ifeq (32,$(port_bits))
libjraiapi2_dbjs  = $(libjraiapi2_objs)
libjraiapi2_dibs  = $(dlld)/libraiapi2$(dll) $(dlld)/libraimsg$(dll) \
                    $(dlld)/libraibase$(dll)
libjraiapi2_dlnk  = -L$(dlld) -lraiapi2 -lraimsg -lraibase
$(dlld)/libjraiapi2$(dll): $(libjraiapi2_dbjs) $(libjraiapi2_dibs)
all_dlls += $(dlld)/libjraiapi2$(dll)
else
libjraiapi232_dbjs  = $(libjraiapi2_objs)
libjraiapi232_dibs  = $(dlld)32/libraiapi2$(dll) $(dlld)32/libraimsg$(dll) \
                      $(dlld)32/libraibase$(dll)
libjraiapi232_dlnk  = -L$(dlld)32 -lraiapi2 -lraimsg -lraibase
$(dlld)32/libjraiapi2$(dll): $(libjraiapi232_dbjs)
all_dlls += $(dlld)32/libjraiapi2$(dll)
endif
endif

ifeq (windows,$(archx))
ifeq (win32,$(arch))
jraiapi2_dbjs  = $(libjraiapi2_objs)
jraiapi2_dibs  = $(dlld)/libraiapi2$(dll) $(dlld)/libraimsg$(dll) \
                 $(dlld)/libraibase$(dll) $(dlld)/libraicache$(dll)
jraiapi2_ntlnk = $(sock_lib)

$(dlld)/jraiapi2$(dll): $(jraiapi2_dbjs) $(jraiapi2_dibs)
all_dlls += $(dlld)/jraiapi2$(dll)
else
jraiapi232_dbjs  = $(libjraiapi2_objs)
jraiapi232_dibs  = $(dlld)32/libraiapi2$(dll) $(dlld)32/libraimsg$(dll) \
                 $(dlld)32/libraibase$(dll) $(dlld)32/libraicache$(dll)
jraiapi232_ntlnk = $(sock_lib)

$(dlld)32/jraiapi2$(dll): $(jraiapi232_dbjs) $(jraiapi232_dibs)
all_dlls += $(dlld)32/jraiapi2$(dll)
endif
endif

all_depends += $(libjraiapi2_deps)
endif

ifeq (64,$(port_bits))
#
# libjraiapi264
#
libjraiapi264_dbjs  = $(java_classd)/$(jraiapi2_root)/rai_api_jni$(fpic)
libjraiapi264_deps  = $(src_dependd)/raiapi/java/$(jraiapi2_root)/rai_api_jni.fpic.d

ifdef unix
libjraiapi264_libs  = 
libjraiapi264_dlnk  = $(math_lib) $(thread_lib)
libjraiapi264_dibs  = $(dlld)/libraiapi2$(dll) $(dlld)/libraimsg$(dll) \
                      $(dlld)/libraibase$(dll)
libjraiapi264_dlnk  = -L$(dlld) -lraiapi2 -lraimsg -lraibase
$(dlld)/libjraiapi264$(dll): $(libjraiapi264_dbjs) $(libjraiapi264_dibs)
all_dlls += $(dlld)/libjraiapi264$(dll)
endif

ifeq ($(arch),win64)
jraiapi264_dbjs  = $(libjraiapi264_dbjs)
jraiapi264_dibs  = $(dlld)/libraiapi2$(dll) $(dlld)/libraimsg$(dll) \
                   $(dlld)/libraibase$(dll) $(dlld)/libraicache$(dll)
jraiapi264_ntlnk = $(sock_lib)
$(dlld)/jraiapi264$(dll): $(jraiapi264_dbjs) $(jraiapi264_dibs)
all_dlls += $(dlld)/jraiapi264$(dll)
endif

all_depends += $(libjraiapi264_deps)
endif
#
# raiapi2.jar
#
raiapi2_root    = $(jraiapi2_root)
raiapi2_classes = $(java_classd)/$(raiapi2_root)/Args.class \
		  $(java_classd)/$(raiapi2_root)/BoolArg.class \
		  $(java_classd)/$(raiapi2_root)/DoubleArg.class \
		  $(java_classd)/$(raiapi2_root)/IntArg.class \
		  $(java_classd)/$(raiapi2_root)/RaiApi.class \
		  $(java_classd)/$(raiapi2_root)/RaiApiException.class \
		  $(java_classd)/$(raiapi2_root)/RaiDataLossCallback.class \
		  $(java_classd)/$(raiapi2_root)/RaiDataLossEvent.class \
		  $(java_classd)/$(raiapi2_root)/RaiConnectionEvent.class \
		  $(java_classd)/$(raiapi2_root)/RaiDict.class \
		  $(java_classd)/$(raiapi2_root)/RaiEntitlement.class \
		  $(java_classd)/$(raiapi2_root)/RaiInteractivePublish.class \
		  $(java_classd)/$(raiapi2_root)/RaiMsgCallback.class \
		  $(java_classd)/$(raiapi2_root)/RaiMsgEvent.class \
		  $(java_classd)/$(raiapi2_root)/RaiPublish.class \
		  $(java_classd)/$(raiapi2_root)/RaiQueue.class \
		  $(java_classd)/$(raiapi2_root)/RaiSession.class \
		  $(java_classd)/$(raiapi2_root)/RaiSubscribeCallback.class \
		  $(java_classd)/$(raiapi2_root)/RaiSubscribe.class \
		  $(java_classd)/$(raiapi2_root)/RaiSubscribeEvent.class \
		  $(java_classd)/$(raiapi2_root)/RaiTimerCallback.class \
		  $(java_classd)/$(raiapi2_root)/RaiTimer.class \
		  $(java_classd)/$(raiapi2_root)/StringArg.class \
		  $(java_classd)/$(raiapi2_root)/Time.class \
		  $(java_classd)/$(raiapi2_root)/TimeRotate.class \
		  $(java_classd)/$(raiapi2_root)/RaiService.class \
		  $(java_classd)/$(raiapi2_root)/RaiServiceFactory.class \
		  $(java_classd)/$(raiapi2_root)/RaiServiceProto.class \
		  $(java_classd)/$(raiapi2_root)/RaiCacheProto.class \
		  $(java_classd)/$(raiapi2_root)/RaiCacheConfig.class \
		  $(java_classd)/$(raiapi2_root)/RaiCacheSvc.class

raiapi2_manifest = $(java_classd)/$(raiapi2_root)/raiapi2_manifest.txt
$(raiapi2_manifest): $(java_srcd)/$(raiapi2_root)/raiapi2.ver

$(libd)/raiapi2.jar: $(raiapi2_manifest) $(raiapi2_classes)

all_libs += $(libd)/raiapi2.jar

