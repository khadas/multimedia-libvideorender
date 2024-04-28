OUT_DIR ?= .
$(info "OUT_DIR : $(OUT_DIR)")

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

OBJ_VIDEOTUNNEL_DISPLAY = \
	videotunnel_lib_wrap.o \
	videotunnel_impl.o \
	videotunnel_plugin.o

OBJ_CLIENT_LIB += $(OBJ_VIDEOTUNNEL_DISPLAY)

LOCAL_CFLAGS += \
	-I../ \
	-I$(TOOLS_PATH) \
	-I$(STAGING_DIR)/usr/include \
	$(VERSION_CFLAGS) \

OBJ_CLIENT_LIB += \
	$(TOOLS_PATH)/Thread.o \
	$(TOOLS_PATH)/Poll.o \
	$(TOOLS_PATH)/Times.o \
	$(TOOLS_PATH)/Logger.o

LOCAL_CFLAGS += -fPIC -O -Wcpp -g

CFLAGS += $(LOCAL_CFLAGS)
CXXFLAGS += $(LOCAL_CFLAGS) -std=c++11

TARGET = $(CLIENT_LIB)

all: $(GENERATED_SOURCES) $(TARGET)

LD_FLAG = -g -fPIC -O -Wcpp -lm -lpthread -lz -Wl,-Bsymbolic -ldl
LD_FLAG_LIB = $(LD_FLAG) -shared $(LD_SUPPORT) -llog

%.o:%.c $(DEPS)
	echo CC $(OUT_DIR)/$@ $(OUT_DIR)/$< $(FLAGS)
	$(CC) -c -o $(OUT_DIR)/$@ $(OUT_DIR)/$< $(CFLAGS) -fPIC

%.o:%.cpp $(DEPS)
	echo CXX $(OUT_DIR)/$@ $< $(FLAGS)
	$(CXX) -c -o $(OUT_DIR)/$@ $< $(CXXFLAGS) -fPIC

$(CLIENT_LIB): $(OBJ_CLIENT_LIB)
	$(CXX) -o $(OUT_DIR)/$@ $(patsubst %, $(OUT_DIR)/%, $^) $(LD_FLAG_LIB)
	cp -f $(OUT_DIR)/$(CLIENT_LIB) $(STAGING_DIR)/usr/lib
	rm -f $(patsubst %, $(OUT_DIR)/%, $(OBJ_CLIENT_LIB))


.PHONY: install
install:
	echo $(TARGET_DIR)
	cp -f $(OUT_DIR)/$(CLIENT_LIB) $(TARGET_DIR)/usr/lib/

.PHONY: clean

clean:
	rm $(OUT_DIR)/$(CLIENT_LIB)
	rm -f $(patsubst %, $(OUT_DIR)/%, $(OBJ_VIDEOTUNNEL_DISPLAY))
	rm -f $(OUT_DIR)/*.o
