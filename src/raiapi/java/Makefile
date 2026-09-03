#
## GNU Makefile (gmake) to build rai Java sample code
#

# Change to point to the Rai installation directory
RAI_DIR=/usr/rai
# If above isn't installed, then this will get include and lib from c++ dir
REL_RAI_DIR=../..
# Change to point to the TIBCO Rendezvous >= 7 installation directory
TIBRV_DIR=/usr/tibco/tibrv

# run these programs with java -Xrs -classpath .:$(PA_CPATH) raisub2
# or java -Xrs -classpath ./raisub2.jar:$(PA_CPATH) raisub2
# where PA_CPATH is defined below;  the -Xrs reduces signals that
# java uses for shutdown, since the programs handle these already
# also set
# LD_LIBRARY_PATH=$(REL_RAI_DIR)/java:$(RAI_DIR)/java:$(REL_RAI_DIR)/dll:$(RAI_DIR)/dll:$(REL_RAI_DIR)/dll32:$(RAI_DIR)/dll32
PA_CLASSES=raisub2.jar raipub2.jar raireplay2.jar raiping2.jar

# run these programs with java -classpath .:$(SC_CPATH) raisub
# where SC_CPATH is defined below
# also set
# LD_LIBRARY_PATH=$(REL_RAI_DIR)/java:$(RAI_DIR)/java:$(TIBRV_DIR)/lib
SC_CLASSES=raisub.class raipub.class raiping.class 

# checks both relative and the install dir, only one is necessary, but if there is
# another installaltion, then the relative dir will override the installation
PA_CPATH=$(REL_RAI_DIR)/java/raiapi2.jar:$(RAI_DIR)/java/raiapi2.jar:$(REL_RAI_DIR)/java/raimsg.jar:$(RAI_DIR)/java/raimsg.jar:$(REL_RAI_DIR)/java/raiexception.jar:$(RAI_DIR)/java/raiexception.jar

# simple santa clara api path, built on top of PA API2
SC_CPATH=$(REL_RAI_DIR)/java/raiapi.jar:$(RAI_DIR)/java/raiapi.jar:$(PA_CPATH)

all: $(PA_CLASSES) $(SC_CLASSES)

raisub2.jar: raisub2.java
	javac -classpath $(PA_CPATH) raisub2.java
	jar cvf raisub2.jar raisub2.class raisub2Args.class 'raisub2$$SubThread.class' rai_service_jraisub2.class

raipub2.jar: raipub2.java
	javac -classpath $(PA_CPATH) raipub2.java
	jar cvf raipub2.jar raipub2.class raipub2Args.class rai_service_jraipub2.class

raireplay2.jar: raireplay2.java
	javac -classpath $(PA_CPATH) raireplay2.java
	jar cvf raireplay2.jar raireplay2.class raireplay2Args.class rai_service_jraireplay2.class

raiping2.jar: raiping2.java
	javac -classpath $(PA_CPATH) raiping2.java
	jar cvf raiping2.jar raiping2.class raiping2Args.class 'raiping2$$PubThread.class' rai_service_jraiping2.class

raisub.class: raisub.java
	javac -classpath $(SC_CPATH) raisub.java

raipub.class: raipub.java
	javac -classpath $(SC_CPATH) raipub.java

raiping.class: raiping.java
	javac -classpath $(SC_CPATH) raiping.java

clean:
	rm -f *.class *.jar
