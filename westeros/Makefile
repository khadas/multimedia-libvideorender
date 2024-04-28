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

$(info ***makefile support wayland->$(WAYLAND_SUPPORT))
#buildroot or local
ifeq ($(WAYLAND_SUPPORT),n)
WAYLAND_SUPPORT=n
else
WAYLAND_SUPPORT=y
endif

#path set
SCANNER_TOOL ?= wayland-scanner
PROTOCOL_PATH = wayland-protocol

GENERATED_SOURCES = \
	$(PROTOCOL_PATH)/linux-dmabuf-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/linux-dmabuf-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/fullscreen-shell-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/fullscreen-shell-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/linux-explicit-synchronization-unstable-v1-protocol.c \
	$(PROTOCOL_PATH)/linux-explicit-synchronization-unstable-v1-client-protocol.h \
	$(PROTOCOL_PATH)/xdg-shell-protocol.c \
	$(PROTOCOL_PATH)/xdg-shell-client-protocol.h \
	$(PROTOCOL_PATH)/viewporter-protocol.c \
	$(PROTOCOL_PATH)/viewporter-client-protocol.h \
	$(PROTOCOL_PATH)/vpc-protocol.c \
	$(PROTOCOL_PATH)/vpc-client-protocol.h \
	$(PROTOCOL_PATH)/vpc-server-protocol.h \
	$(PROTOCOL_PATH)/simplebuffer-protocol.c \
	$(PROTOCOL_PATH)/simplebuffer-client-protocol.h \
	$(PROTOCOL_PATH)/simplebuffer-server-protocol.h \
	$(PROTOCOL_PATH)/simpleshell-protocol.c \
	$(PROTOCOL_PATH)/simpleshell-client-protocol.h \
	$(PROTOCOL_PATH)/simpleshell-server-protocol.h \
	$(PROTOCOL_PATH)/wayland-client-protocol.h

OBJ_WESTEROS_DISPLAY = \
	wstclient_wayland.o \
	wstclient_socket.o \
	wstclient_plugin.o \
	wst_essos.o

LOCAL_CFLAGS += \
	-I$(OUT_DIR)/$(PROTOCOL_PATH)

ifeq ($(WAYLAND_SUPPORT), y)
LOCAL_CFLAGS += -DSUPPORT_WAYLAND
OBJ_CLIENT_LIB += $(OBJ_WESTEROS_DISPLAY)
OBJ_CLIENT_LIB += $(GENERATED_SOURCES:.c=.o)

LD_SUPPORT += -lwayland-client
endif

LOCAL_CFLAGS += \
	-I../ \
	-I$(TOOLS_PATH) \
	-I$(STAGING_DIR)/usr/include \
	$(VERSION_CFLAGS) \

OBJ_CLIENT_LIB += \
	$(TOOLS_PATH)/Thread.o \
	$(TOOLS_PATH)/Poll.o \
	$(TOOLS_PATH)/Times.o \
	$(TOOLS_PATH)/Utils.o \
	$(TOOLS_PATH)/Logger.o

LOCAL_CFLAGS += -fPIC -O -Wcpp -g

CFLAGS += $(LOCAL_CFLAGS)
CXXFLAGS += $(LOCAL_CFLAGS) -std=c++11

TARGET = $(CLIENT_LIB)

all: $(GENERATED_SOURCES) $(TARGET)

LD_FLAG = -g -fPIC -O -Wcpp -lm -lpthread -lz -Wl,-Bsymbolic -ldl
LD_FLAG_LIB = $(LD_FLAG) -shared $(LD_SUPPORT) -llog -lessosrmgr

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

$(PROTOCOL_PATH)/%-protocol.c : $(PROTOCOL_PATH)/%.xml
	echo $(@D)
	mkdir -p $(@D) && $(SCANNER_TOOL) public-code < $< > $(OUT_DIR)/$@

$(PROTOCOL_PATH)/%-server-protocol.h : $(PROTOCOL_PATH)/%.xml
	mkdir -p $(@D) && $(SCANNER_TOOL) server-header < $< > $(OUT_DIR)/$@

$(PROTOCOL_PATH)/%-client-protocol.h : $(PROTOCOL_PATH)/%.xml
	mkdir -p $(@D) && $(SCANNER_TOOL) client-header < $< > $(OUT_DIR)/$@


.PHONY: install
install:
	echo $(TARGET_DIR)
	cp -f $(OUT_DIR)/$(CLIENT_LIB) $(TARGET_DIR)/usr/lib/

.PHONY: clean

clean:
	rm $(OUT_DIR)/$(CLIENT_LIB)
	rm -f $(OUT_DIR)/$(PROTOCOL_PATH)/*.o
	rm -f $(patsubst %, $(OUT_DIR)/%, $(OBJ_WESTEROS_DISPLAY))
	rm -f $(OUT_DIR)/*.o

$(shell mkdir -p $(OUT_DIR)/wayland-protocol)
$(shell mkdir -p $(OUT_DIR)/$(TOOLS_PATH))