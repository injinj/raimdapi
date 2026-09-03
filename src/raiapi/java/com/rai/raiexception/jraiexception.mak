#
#  raiexception.jar
#
raiexception_root  = com/rai/raiexception

raiexception_classes = $(java_classd)/$(raiexception_root)/RaiException.class

raiexception_manifest = $(java_classd)/$(raiexception_root)/raiexception_manifest.txt
$(raiexception_manifest): $(java_srcd)/$(raiexception_root)/raiexception.ver

$(libd)/raiexception.jar: $(raiexception_manifest) $(raiexception_classes)

all_libs += $(libd)/raiexception.jar

