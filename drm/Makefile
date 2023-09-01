SHELL := /bin/bash
BRANCH_NAME="$(shell cd $(SRC_PATH);git branch -a | sed -n '/'*'/p')"
GIT_VERSION :="$(shell cd $(SRC_PATH);git log | grep commit -m 1 | cut -d' ' -f 2)"
COMMIT_CHANGEID="$(shell cd $(SRC_PATH);git log | grep Change-Id -m 1)"

COMMIT_PD :="$(shell cd $(SRC_PATH);git log | grep PD\# -m 1)"
LAST_CHANGED="$(shell cd $(SRC_PATH);git log | grep Date -m 1)"
BUILD_TIME=" $(shell date)"
BUILD_NAME=" $(shell echo ${LOGNAME})"

VERSION_CFLAGS := -DHAVE_VERSION_INFO
VERSION_CFLAGS+=-DBRANCH_NAME=\"${BRANCH_NAME}\"
VERSION_CFLAGS+=-DGIT_VERSION=\"${GIT_VERSION}${GIT_DIRTY}\"
VERSION_CFLAGS+=-DCOMMIT_CHANGEID=\"${COMMIT_CHANGEID}\"
VERSION_CFLAGS+=-DCOMMIT_PD=\"${COMMIT_PD}\"
VERSION_CFLAGS+=-DLAST_CHANGED=\"${LAST_CHANGED}\"
VERSION_CFLAGS+=-DBUILD_TIME=\"${BUILD_TIME}\"
VERSION_CFLAGS+=-DBUILD_NAME=\"${BUILD_NAME}\"

CLIENT_LIB = libvideorender_client.so

TOOLS_PATH = ../tools

#default supporting drm freezen feature
SUPPORT_DRM_FREEZEN=y
#roku platform has no wayland so
ifeq ($(CC_TARGET_FLOAT_ABI_),softfp)
SUPPORT_DRM_FREEZEN=n
endif

ifeq ($(SUPPORT_DRM_FREEZEN), y)
LOCAL_CFLAGS += -DSUPPORT_DRM_FREEZEN
endif

OBJ_DRM_DISPLAY = \
	drm_lib_wrap.o \
	drm_framepost.o \
	drm_framerecycle.o \
	drm_display.o \
	drm_plugin.o


OBJ_CLIENT_LIB += $(OBJ_DRM_DISPLAY)

LOCAL_CFLAGS += \
	-I../ \
	-I$(TOOLS_PATH) \
	-I$(STAGING_DIR)/usr/include \
	-I$(STAGING_DIR)/usr/include/libdrm_meson \
	-I$(STAGING_DIR)/usr/include/libdrm \
	$(VERSION_CFLAGS) \

OBJ_CLIENT_LIB += \
	$(TOOLS_PATH)/Thread.o \
	$(TOOLS_PATH)/Times.o \
	$(TOOLS_PATH)/Logger.o \
	$(TOOLS_PATH)/Queue.o

LOCAL_CFLAGS += -fPIC -O -Wcpp -g

CFLAGS += $(LOCAL_CFLAGS)
CXXFLAGS += $(LOCAL_CFLAGS) -std=c++11

TARGET = $(CLIENT_LIB)

all: $(GENERATED_SOURCES) $(TARGET)

LD_FLAG = -g -fPIC -O -Wcpp -lm -lpthread -lz -Wl,-Bsymbolic -ldl
LD_FLAG_LIB = $(LD_FLAG) -shared $(LD_SUPPORT) -llog -ldrm

%.o:%.c $(DEPS)
	echo CC $@ $< $(FLAGS)
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

%.o:%.cpp $(DEPS)
	echo CXX $@ $< $(FLAGS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) -fPIC

$(CLIENT_LIB): $(OBJ_CLIENT_LIB)
	$(CXX) -o $@ $^ $(LD_FLAG_LIB)
	cp -f $(CLIENT_LIB) $(STAGING_DIR)/usr/lib
	rm -f $(OBJ_CLIENT_LIB)

.PHONY: install
install:
	echo $(TARGET_DIR)
	cp -f $(CLIENT_LIB) $(TARGET_DIR)/usr/lib/

.PHONY: clean

clean:
	rm $(CLIENT_LIB)
	rm -f *.o
