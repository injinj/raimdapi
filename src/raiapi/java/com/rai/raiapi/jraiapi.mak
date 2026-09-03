#
#  raiapi.jar
#
raiapi_root  = com/rai/raiapi
raiapi_classes = $(java_classd)/$(raiapi_root)/RaiApi.class \
                 $(java_classd)/$(raiapi_root)/RaiCallback.class \
                 $(java_classd)/$(raiapi_root)/RaiException.class \
                 $(java_classd)/$(raiapi_root)/RaiEvent.class \
                 $(java_classd)/$(raiapi_root)/RaiSubscribe.class \
                 $(java_classd)/$(raiapi_root)/SubHandle.class \
                 $(java_classd)/$(raiapi_root)/RaiCallback.class \
                 $(java_classd)/$(raiapi_root)/RaiTimerCallback.class \
                 $(java_classd)/$(raiapi_root)/RaiPublish.class \
                 $(java_classd)/$(raiapi_root)/TimerHandle.class \
                 $(java_classd)/$(raiapi_root)/RaiDict.class \
                 $(java_classd)/$(raiapi_root)/RaiSession.class \
                 $(java_classd)/$(raiapi_root)/RaiTimer.class

raiapi_manifest = $(java_classd)/$(raiapi_root)/raiapi_manifest.txt
$(raiapi_manifest): $(java_srcd)/$(raiapi_root)/raiapi.ver

$(libd)/raiapi.jar: $(raiapi_manifest) $(raiapi_classes)

all_libs += $(libd)/raiapi.jar

