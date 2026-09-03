#
# libjraimsg
#
jraimsg_root  = com/rai/raimsg

ifneq (win64,$(arch))
rai_msg_jni_includes = -I$(java_home)/include -I$(java_home)/include/$(arch)
else
rai_msg_jni_includes = -I$(java_home)/include -I$(java_home)/include/win32
endif

ifeq (32,$(port_bits))
java_build32_bits=true
endif
ifeq (64_32,$(build_bits))
java_build32_bits=true
endif

ifeq (true,$(java_build32_bits))
libjraimsg_objs  = $(java_classd)/$(jraimsg_root)/rai_msg_jni$(fpic32)
#                   $(addprefix $(src_objd)/msg/, $(addsuffix $(fpic32), $(notdir $(libraimsg_files))))
#                   $(addprefix $(src_objd)/base/, $(addsuffix $(fpic32), $(notdir $(base_files))))
#                   $(addprefix $(src_objd)/stream/, $(addsuffix $(fpic32), $(notdir $(stream_files))))
#                   $(addprefix $(src_objd)/util/, $(addsuffix $(fpic32), $(notdir $(util_files))))
libjraimsg_deps  = $(src_dependd)/raiapi/java/$(jraimsg_root)/rai_msg_jni.fpic.32.d
#                   $(addprefix $(src_dependd)/msg/, $(addsuffix .fpic.32.d, $(notdir $(libraimsg_files))))
#                   $(addprefix $(src_dependd)/base/, $(addsuffix .fpic.32.d, $(notdir $(base_files))))
#                   $(addprefix $(src_dependd)/stream/, $(addsuffix .fpic.32.d, $(notdir $(stream_files))))
#                   $(addprefix $(src_dependd)/util/, $(addsuffix .fpic.32.d, $(notdir $(util_files))))

ifdef unix
ifeq (32,$(port_bits))
libjraimsg_dbjs = $(libjraimsg_objs)
libjraimsg_dibs  = $(dlld)/libraimsg$(dll) $(dlld)/libraibase$(dll)
libjraimsg_dlnk  = -L$(dlld) -lraimsg -lraibase
$(dlld)/libjraimsg$(dll): $(libjraimsg_dbjs) $(libjraimsg_dibs)
all_dlls += $(dlld)/libjraimsg$(dll)
else
libjraimsg32_dbjs = $(libjraimsg_objs)
libjraimsg32_dibs = $(dlld)32/libraimsg$(dll) $(dlld)32/libraibase$(dll)
libjraimsg32_dlnk = -L$(dlld)32 -lraimsg -lraibase
$(dlld)32/libjraimsg$(dll): $(libjraimsg32_dbjs) $(libjraimsg32_dibs)
all_dlls += $(dlld)32/libjraimsg$(dll)
endif
endif

ifeq (windows,$(archx))
ifeq ($(arch),win32)
jraimsg_dbjs  = $(libjraimsg_objs)
jraimsg_dibs  = $(dlld)/libraimsg$(dll) $(dlld)/libraibase$(dll)
jraimsg_ntlnk = $(sock_lib)
$(dlld)/jraimsg$(dll): $(jraimsg_dbjs) $(jraimsg_dibs)
all_dlls += $(dlld)/jraimsg$(dll)
else
jraimsg32_dbjs  = $(libjraimsg_objs)
jraimsg32_dibs  = $(dlld)32/libraimsg$(dll) $(dlld)32/libraibase$(dll)
jraimsg32_ntlnk = $(sock_lib)
$(dlld)32/jraimsg$(dll): $(jraimsg32_dbjs) $(jraimsg32_dibs)
all_dlls += $(dlld)32/jraimsg$(dll)
endif
endif

all_depends += $(libjraimsg32_deps)
endif

ifeq (64,$(port_bits))
#
# libjraimsg64
#
libjraimsg64_dbjs  = $(java_classd)/$(jraimsg_root)/rai_msg_jni$(fpic)
#                   $(addprefix $(src_objd)/msg/, $(addsuffix $(fpic), $(notdir $(libraimsg_files))))
#                   $(addprefix $(src_objd)/base/, $(addsuffix $(fpic), $(notdir $(base_files))))
#                   $(addprefix $(src_objd)/stream/, $(addsuffix $(fpic), $(notdir $(stream_files))))
#                   $(addprefix $(src_objd)/util/, $(addsuffix $(fpic), $(notdir $(util_files))))
libjraimsg64_deps  = $(src_dependd)/raiapi/java/$(jraimsg_root)/rai_msg_jni.fpic.d
#                   $(addprefix $(src_dependd)/msg/, $(addsuffix .fpic.d, $(notdir $(libraimsg_files))))
#                   $(addprefix $(src_dependd)/base/, $(addsuffix .fpic.d, $(notdir $(base_files))))
#                   $(addprefix $(src_dependd)/stream/, $(addsuffix .fpic.d, $(notdir $(stream_files))))
#                   $(addprefix $(src_dependd)/util/, $(addsuffix .fpic.d, $(notdir $(util_files))))

ifdef unix
libjraimsg64_dibs  = $(dlld)/libraimsg$(dll) $(dlld)/libraibase$(dll)
libjraimsg64_dlnk  = -L$(dlld) -lraimsg -lraibase
$(dlld)/libjraimsg64$(dll): $(libjraimsg64_dbjs) $(libjraimsg64_dibs)
all_dlls += $(dlld)/libjraimsg64$(dll)
endif

ifeq ($(arch),win64)
jraimsg64_dbjs  = $(libjraimsg64_dbjs)
jraimsg64_dibs  = $(dlld)/libraimsg$(dll) $(dlld)/libraibase$(dll)
jraimsg64_ntlnk = $(sock_lib)
$(dlld)/jraimsg64$(dll): $(jraimsg64_dbjs) $(jraimsg64_dibs)
all_dlls += $(dlld)/jraimsg64$(dll)
endif

all_depends += $(libjraimsg64_deps)
endif
#
# raimsg.jar
#
raimsg_root    = $(jraimsg_root)
raimsg_classes = $(java_classd)/$(raimsg_root)/Partial.class \
                 $(java_classd)/$(raimsg_root)/RaiField.class \
                 $(java_classd)/$(raimsg_root)/RaiMsgException.class \
                 $(java_classd)/$(raimsg_root)/RaiMsg.class \
                 $(java_classd)/$(raimsg_root)/SassConst.class

raimsg_manifest = $(java_classd)/$(raimsg_root)/raimsg_manifest.txt
$(raimsg_manifest): $(java_srcd)/$(raimsg_root)/raimsg.ver

$(libd)/raimsg.jar: $(raimsg_manifest) $(raimsg_classes)

all_libs += $(libd)/raimsg.jar

